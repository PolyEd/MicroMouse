import pygame
pygame.init()

### Setup important vars

x = 14
y = 14

file = open('nmaze.txt','r')
lines = file.readlines()
walls = lines[1:]; walls = [((int(wall.split('),(')[0].split(',')[0][1:]), int(wall.split('),(')[0].split(',')[1])),(int(wall.split('),(')[1].split(',')[0]), int(wall.split('),(')[1].split(',')[1].strip()[:-1]))) for wall in walls]
owalls = [((0,0),(x,0)),((x,0),(x,y)),((x,y),(0,y)),((0,y),(0,0))]

sx, sy = (0,0)
goals = [(6,6),(6,7),(7,6),(7,7)]
goal = (7,7) ### once it goes thro the maze, it will know which goal it can enter
area = x*y

blackdots = [(7,7),(13,13)]

CELLSIZE = 45
w, h = (x+2)*CELLSIZE, (y+2)*CELLSIZE

screen = pygame.display.set_mode((w,h),flags=pygame.RESIZABLE)
clock = pygame.time.Clock()

### Check for wall function

def iswall(x1,y1,x2,y2):
    for wall in walls+owalls:
        xs = wall[0][0]; xe = wall[1][0]
        ys = wall[0][1]; ye = wall[1][1]
        if wall[0][0] > wall[1][0]: xs = wall[1][0]; xe = wall[0][0]    
        if wall[0][1] > wall[1][1]: ys = wall[1][1]; ye = wall[0][1]
        if xs<=x1<=xe and ys<=y1<=ye and xs<=x2<=xe and ys<=y2<=ye: return 1
    return 0

### Generate distance matrix function

def gendist(walls):
    distmat = []
    for i in range(y): distmat.append([area+1]*x)
    updt = 4
    curdist = 1
    for goal in goals: distmat[goal[1]][goal[0]] = 0
    nxs = []
    mxs = goals
    k = 1
    while updt < area and k:
        lastupdt = updt
        nxs = mxs
        mxs = []
        for (curx,cury) in nxs:
            if (not iswall(curx,cury,curx,cury+1)) and (distmat[cury][curx-1] == area+1 and (curx-1,cury) not in goals): mxs.append((curx-1,cury)); distmat[cury][curx-1] = curdist; updt +=1
            if (not iswall(curx+1,cury,curx+1,cury+1)) and (distmat[cury][curx+1] == area+1 and (curx+1,cury) not in goals): mxs.append((curx+1,cury)); distmat[cury][curx+1] = curdist; updt +=1
            if (not iswall(curx,cury,curx+1,cury)) and (distmat[cury-1][curx] == area+1 and (curx,cury-1) not in goals): mxs.append((curx,cury-1)); distmat[cury-1][curx] = curdist; updt +=1
            if (not iswall(curx,cury+1,curx+1,cury+1)) and (distmat[cury+1][curx] == area+1 and (curx,cury-1) not in goals): mxs.append((curx,cury+1)); distmat[cury+1][curx] = curdist; updt +=1
        curdist +=1
        if lastupdt == updt: k = 0
    return distmat

### Key listener class (linked to display)

class actionListener:
    def __init__(self):
        self.listens = []
        keys = len(pygame.key.get_pressed())
        self.downs = [False]*keys
        self.curs = [False]*keys
        
    def addListener(self, key, action):
        self.listens.append([pygame.key.key_code(key),action])

    def update(self, curkeys):
        for (key, action) in self.listens:
            self.curs[key] = self.downs[key]
            if curkeys[key]: self.downs[key] = True
            else: self.downs[key] = False; self.curs[key] = False
            if self.downs[key] is not self.curs[key]:
                action()

### Display class

class display:
    def __init__(self, screen):
        self.screen = screen
        self.points = False
        self.nums = True
        self.special = True
        self.walls = True
        self.route = True
        self.font = pygame.font.SysFont('arialunicode',19)
        self.distmat = gendist(walls)
        self.rroute = solve(sx,sy,goal,self.distmat)

    def togglepoints(self): self.points = not self.points
    def togglenums(self): self.nums = not self.nums
    def togglespecial(self): self.special = not self.special
    def togglewalls(self): self.walls = not self.walls
    def toggleroute(self): self.route = not self.route

    def update(self):
        if self.special:
            pygame.draw.rect(self.screen,(0,255,0),pygame.Rect((sx+1)*CELLSIZE,(sy+1)*CELLSIZE,CELLSIZE,CELLSIZE))
            for goal in goals:
                pygame.draw.rect(self.screen,(245,239,66),pygame.Rect((goal[0]+1)*CELLSIZE,(goal[1]+1)*CELLSIZE,CELLSIZE,CELLSIZE))
        if self.nums:
            for i in range(y):
                for j in range(x):
                    text = self.font.render(str(self.distmat[j][i]),True,(217,108,46))
                    self.screen.blit(text,((i+1.5)*CELLSIZE-text.get_width()/2,(j+1.5)*CELLSIZE-text.get_height()/2))
        if self.points:
            for i in range(1,x+2):
                for j in range(1,y+2):
                    if (i-1,j-1) in blackdots: c = (0,0,0)
                    else: c = (255,20,40)
                    pygame.draw.circle(self.screen,c,(i*CELLSIZE,j*CELLSIZE),3)
        if self.walls:
            for wall in walls + owalls:
                xs, ys = wall[0]
                xe, ye = wall[1]
                pygame.draw.line(self.screen,(0,0,0),((xs+1)*CELLSIZE,((ys+1)*CELLSIZE)),((xe+1)*CELLSIZE,(ye+1)*CELLSIZE))
        if self.route:
            for p in range(1,len(self.rroute)):
                pygame.draw.line(self.screen,(3,36,252),((self.rroute[p-1][0]+1.5)*CELLSIZE,(self.rroute[p-1][1]+1.5)*CELLSIZE),((self.rroute[p][0]+1.5)*CELLSIZE,(self.rroute[p][1]+1.5)*CELLSIZE),4)

        
def solve(sx,sy,goal,distmat):
    route = [(sx,sy)]
    cx = sx
    cy = sy
    while (cx,cy) != goal:
        best = (cx,cy)
        if (not iswall(cx,cy,cx,cy+1)) and distmat[cy][cx-1] < distmat[best[1]][best[0]]: best = (cx-1,cy)
        if (not iswall(cx+1,cy,cx+1,cy+1)) and distmat[cy][cx+1] < distmat[best[1]][best[0]]: best = (cx+1,cy) 
        if (not iswall(cx,cy,cx+1,cy)) and distmat[cy-1][cx] < distmat[best[1]][best[0]]: best = (cx,cy-1)
        if (not iswall(cx,cy+1,cx+1,cy+1)) and distmat[cy+1][cx] < distmat[best[1]][best[0]]: best = (cx,cy+1)
        cx, cy = best
        route.append(best)
    return route

                
### init a display class
d = display(screen)


### init a key listener class
a = actionListener()

### link keys to display toggles
a.addListener('p', d.togglepoints)
a.addListener('n', d.togglenums)
a.addListener('s', d.togglespecial)
a.addListener('w', d.togglewalls)
a.addListener('r', d.toggleroute)

running = True

#main loop

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT: exit()

    screen.fill((255,255,255))

    keys = pygame.key.get_pressed()
    a.update(keys)

    d.update()
    
    clock.tick(60)
    pygame.display.update()
