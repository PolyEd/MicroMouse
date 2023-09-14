import pygame
import sys
import time
import os
import random
import math

from createmaze import genmaze

pygame.init()

taken = []

files = os.listdir()
for file in files:
    if 'maze' in file and '.txt' in file: taken.append(int(file.split('maze')[1].split('.')[0]))

taken.sort()

print('\nPlease enter a valid maze number (a positive integer) \n' + '(Keep in mind the following numbers maze numbers already have maze files: ' + str(taken) + ')\nMaze number --> ', end='')
mazenum = input('')
if not mazenum: mazenum = taken[-1] + 1
if not (int(mazenum) in taken):
    print('\nGenerating maze: ' + str(mazenum))
    genmaze(mazenum)
    for i in range(17 + len(str(mazenum))):
        print('#',end='')
        time.sleep(random.randint(4,20)/100)
    print('\n\nSucessfully generated \'maze' + str(mazenum) + '.txt\'')
openmessage = 'Opening maze file: \'maze'+str(mazenum)+'.txt\''
print('\n' + openmessage)
mazefile = open('maze'+str(mazenum)+'.txt','r')
for i in range(len(openmessage)):
    print('#',end='')
    time.sleep(random.randint(4,20)/100)
print('\n\nSucessfully opened \'maze' + str(mazenum) + '.txt\'')

print('\nSolving maze now',end='')
for i in range(3):
    print('.', end='')
    time.sleep(0.4)

print('\n')

time.sleep(0.2)

lines = mazefile.readlines()
CELLSIZE = int(lines[0].strip().split(':')[1].strip())
centerstr = lines[1].strip().split(':')[1].strip()
cx,cy = centerstr.split(',')
cx = int(cx.split('(')[1])
cy = int(cy.split(')')[0])
center = (cx,cy)

fontsize=int(lines[2].strip().split(':')[1].strip())
playersize=int(lines[3].strip().split(':')[1].strip())

MAZE = []
maze = lines[5:]
for line in maze:
    MAZE.append([eval(i) for i in line.strip().strip('][').split(', ')])

mazefile.close()


###solver and displayer class
class Player:
    def __init__(self,MAZE,pos:tuple,center:tuple,ori:tuple,cellsize,screendelay,fontsize,size=20):
        self.MAZE = MAZE
        self.CELLSIZE = cellsize
        self.TICKSPEED = 60
        self.wallc = (0,0,0)
        self.startc = (0,255,0)
        self.goalc = (252,186,3)
        self.notwallyetc = (106, 110, 101)
        self.backgroundc = (72,150,199)
        self.font = pygame.font.SysFont('arialunicode',fontsize)
        self.clock = pygame.time.Clock()
        self.x=pos[0]
        self.y=pos[1]
        self.s = pos
        self.ori = ori
        self.msize = len(self.MAZE)
        self.SIZE = [(self.CELLSIZE*self.msize)-1]*2
        self.screen = pygame.display.set_mode(self.SIZE,flags=pygame.RESIZABLE)
        self.center = center
        self.cx = self.center[0]
        self.cy = self.center[1]
        distmat = []
        for i in range(self.msize): distmat.append([0]*self.msize)
        for y in range(self.msize):
            for x in range(self.msize):
                distmat[y][x] = int(abs(x-self.cx) + abs(y-self.cy))
        self.distmat = distmat
        self.c = (219,76,33)
        self.psize = size
        self.beeline = self.bresenham(self.x,self.y,self.cx,self.cy)
        self.beelinec = (170,66,245)
        self.knownw = []
        for i in range(self.msize):
            self.knownw.append((-1,i))
            self.knownw.append((self.msize,i))
            self.knownw.append((i,-1))
            self.knownw.append((i,self.msize))
        self.knownwin = []
        self.visited = [(self.x,self.y)]
        self.junctions = []
        self.deadends = []
        self.screendelay=screendelay
        self.solved=False
        self.oldknown = []
        self.quad = self.get_quad()
        self.debug = False
        self.keydown = False

    def get_quad(self):
        if self.cx > self.x: x = True
        else: x = False
        if self.cy > self.y: y = True
        else: y = False
        return(x,y)
    
    def is_wall(self,pos:tuple):
        x = pos[0]
        y = pos[1]
        if (x,y) in self.knownw or (x,y) in self.knownwin: return 1
        else: return 0
        
    def update_around(self):
        t = 0
        around = []
        for i in range(-2,2):
            x = (i%2)*i
            if i<0: y = i+1
            else: y = 1-x
            mx = self.x + x
            my = self.y + y
            if (-1<mx<self.msize and -1<my<self.msize):
                if self.MAZE[my][mx]:
                    if (mx,my) not in self.knownw:
                        if (mx,my) not in self.knownwin:
                            self.knownwin.append((mx,my))
            else: t+= 1
        if t>=2: self.junctions.append((self.x,self.y))
        if t==1: self.deadends.append((self.x,self.y))
    
    def gen_dist(self):
        distmat = []
        for i in range(self.msize): distmat.append([0]*self.msize)
        updt = 0
        for (x,y) in self.knownwin: distmat[y][x] = (self.msize**2)+1; updt +=1
        curdist = 0
        curx = self.cx
        cury = self.cy
        distmat[cury][curx] = curdist
        updt += 1
        curdist = 1
        nxs = []
        mxs = [(curx,cury)]
        k = 2
        while updt < self.msize**2 and k:
            if k == 1: k = 0
            lastupdt = updt
            nxs = mxs
            mxs = []
            for nx in nxs:
                curx = nx[0]
                cury = nx[1]
                paround = []
                for i in range(-2,2):
                    x = (i%2)*i
                    if i<0: y = i+1
                    else: y = 1-x
                    mx = curx + x
                    my = cury + y
                    if not self.is_wall((mx,my)): paround.append((mx,my))
                for path in paround:
                    px = path[0]
                    py = path[1]
                    if distmat[py][px] == 0 and (px,py) != self.center:
                        mxs.append((px,py))
                        distmat[py][px] = curdist
                        updt +=1
            curdist +=1
            if lastupdt == updt and k == 2: k = 1
        for y in range(self.msize):
            for x in range(self.msize):
                if distmat[y][x] == 0: distmat[y][x] = (self.msize ** 2) + 1
        distmat[self.cy][self.cx] = 0
        self.distmat = distmat

    def get_dist(self,x,y):
        if x<0 or x>self.msize-1 or y<0 or y>self.msize-1: return (self.msize**2)+1
        else: return self.distmat[y][x]

    def bresenham(self,x1,y1,x2,y2):
        x1 = round(x1)
        y1 = round(y1)
        x2 = round(x2)
        y2 = round(y2)
        
        def sign(x):
            if x == 0: return x
            return x/abs(x)
            
        x = x1
        y = y1
        cx = abs(x2-x1)
        cy = abs(y2-y1)
        s1 = sign(x2-x1)
        s2 = sign(y2-y1)
        if cy > cx:
            t = cx
            cx = cy
            cy = t
            inter = 1
        else:
            inter = 0
        e = (2*cy)-cx
        a = e+cx
        b = e-cx

        points = [(x1,y1)]
        for i in range(1,cx):
            if e < 0:
                if inter:
                    y += s2
                else:
                    x += s1
                    e += a
            else:
                y += s2
                x += s1
                e += b
            points.append((int(x),int(y)))

        points.append((x2,y2))
        return points
    
    def solve(self):
        self.solved=False
        self.gen_dist()
        self.display()
        time.sleep(self.screendelay)
        while (self.x,self.y) != self.center:
            self.update_around()
            self.gen_dist()
            self.display()
            curdist = self.get_dist(self.x,self.y)
            neighbors = []
            for i in range(-2,2):
                x=(i%2)*i
                if i<0: y = i+1
                else: y =1-x
                mx = self.x+x
                my = self.y+y
                neighbors.append(((mx,my),self.get_dist(mx,my)))
            neighbors.sort(key=lambda x:x[1])
            
            self.quad = self.get_quad()
            if self.quad[0]: qx = 0
            else: qx = self.msize-1
            if self.quad[1]: qy = 0
            else: qy = self.msize-1
            self.beeline = self.bresenham(qx,qy,self.cx,self.cy)
            
            mv0,mv1 = neighbors[:2]
            if mv0[1] == mv1[1]:
                if mv0[0] in self.beeline: mv = mv0
                elif mv1[0] in self.beeline: mv = mv1
                else:
                    mv0distb = []
                    mv1distb = []
                    for step in self.beeline:
                        dx0 = abs(mv0[0][0]-step[0])
                        dy0 = abs(mv0[0][1]-step[1])
                        mv0distb.append(dx0+dy0)
                        dx1 = abs(mv1[0][0]-step[0])
                        dy1 = abs(mv1[0][1]-step[1])
                        mv1distb.append(dx1+dy1)
                    mv0dist = min(mv0distb)
                    mv1dist = min(mv1distb)
                    if mv1dist<mv0dist: mv = mv1
                    else: mv = mv0
            else: mv = mv0
            self.x=mv[0][0]
            self.y=mv[0][1]
            time.sleep(self.screendelay)
            
        self.display()
        time.sleep(2)
        self.solved=True
        if not self.debug: self.debug=True; print('Debug mode enabled!')
        self.oldknown = self.knownwin
        self.knownwin = self.get_all_walls()
        self.gen_dist()
        print('\nDone!')
        while 1: self.display()
            
    def drawcell(self, pos: tuple,col,s=0):
        if s: pygame.draw.rect(self.screen,col,pygame.Rect(pos[0]*self.CELLSIZE+self.CELLSIZE/8,pos[1]*self.CELLSIZE+self.CELLSIZE/8,self.CELLSIZE/(4/3),self.CELLSIZE/(4/3)))
        else: pygame.draw.rect(self.screen,col,pygame.Rect(pos[0]*self.CELLSIZE,pos[1]*self.CELLSIZE,self.CELLSIZE-1,self.CELLSIZE-1))
        
    def drawline(self,o,pos):
        if o: l=pygame.Rect(pos*self.CELLSIZE-1,0,1,self.SIZE[0])
        else: l=pygame.Rect(0,pos*self.CELLSIZE-1,self.SIZE[0],1)
        pygame.draw.rect(self.screen,(252,3,240),l)

    def get_all_walls(self):
        walls = []
        for y in range(self.msize):
            for x in range(self.msize):
                if self.MAZE[y][x]: walls.append((x,y))
        return(walls)
            
    def drawplayer(self):
        pygame.draw.rect(self.screen,self.c,pygame.Rect(self.x*self.CELLSIZE+((self.CELLSIZE-self.psize)/2),self.y*self.CELLSIZE+((self.CELLSIZE-self.psize)/2),self.psize,self.psize))
    
    def display(self):
        self.screen.fill(self.backgroundc)
        keys = pygame.key.get_pressed()
        curpress = self.keydown
        if keys[pygame.K_d]: self.keydown = True
        else: self.keydown = False; curpress = False
        if self.keydown is not curpress:
            self.debug = not self.debug
            if self.debug: print('Debug mode enabled!')
            else: print('Debug mode disabled!')

        for event in pygame.event.get():
            if event.type == pygame.VIDEORESIZE:
                self.screen = pygame.display.set_mode((event.w, event.w),pygame.RESIZABLE)
                self.SIZE = [event.w]*2
            if event.type == pygame.QUIT: sys.exit()
        self.CELLSIZE = self.screen.get_width()/self.msize

                
        if self.debug:
            for row in range(self.msize):
                for col in range(self.msize):
                    if self.MAZE[row][col]:
                        self.drawcell((col,row),self.notwallyetc)
            for sq in self.beeline:
                self.drawcell(sq,self.beelinec)
            if not self.solved:
                for wall in self.knownwin:
                    self.drawcell(wall,self.wallc)
            else:
                for wall in self.oldknown:
                    self.drawcell(wall,self.wallc)
        else:
            if not self.solved:
                for wall in self.knownwin:
                    self.drawcell(wall,self.wallc)
            else:
                for wall in self.oldknown:
                    self.drawcell(wall,self.wallc)
                    
        if (self.x,self.y) == (self.cx,self.cy):
            self.beeline = []
                
        self.drawcell(self.s,self.startc,1)
        self.drawcell(self.center,self.goalc,1)
        for row in range(self.msize-1): self.drawline(1,row+1)
        for col in range(self.msize-1): self.drawline(0,col+1)
        
        for row in range(self.msize):
            for col in range(self.msize):
                dist = self.font.render(str(self.get_dist(col,row)),True,(255,255,255))
                self.screen.blit(dist,(col*self.CELLSIZE+self.CELLSIZE/2-dist.get_width()/2,row*self.CELLSIZE+self.CELLSIZE/2-dist.get_height()/2))
        self.drawplayer()
        pygame.display.update()
        self.clock.tick(self.TICKSPEED)


###extra vars
ori = (1,0)
start=(0,0)

screenrefresh = math.trunc(((0.00012027*(len(MAZE)**2))+(-0.0168297*len(MAZE))+0.596649)*100)/100


###pass everything
p = Player(MAZE,start,center,ori,CELLSIZE,screenrefresh,fontsize,playersize)

#print(p.bresenham())
###long anticipated solve
p.solve()
