# Maze generator -- Randomized Prim Algorithm
 
## Imports

import random
import time
import math
from PIL import Image
import numpy as np
from time import gmtime, strftime

MAZEMIN = 1500
MAZEMAX = 1500

print('here we go')
print('started at' + str(strftime("%Y-%m-%d %H:%M:%S", gmtime())) + ' which is +4h')


setsize = 1

if setsize: MAZEMIN = MAZEMAX

wall = 1
cell = 0
unvisited = 2
height = random.randint(MAZEMIN,MAZEMAX)
width = random.randint(MAZEMIN,MAZEMAX)
maze = []


def surroundingCells(rand_wall):
    s_cells = 0
    if (maze[rand_wall[0]-1][rand_wall[1]] == 0):
            s_cells += 1
    if (maze[rand_wall[0]+1][rand_wall[1]] == 0):
            s_cells += 1
    if (maze[rand_wall[0]][rand_wall[1]-1] == 0):
            s_cells +=1
    if (maze[rand_wall[0]][rand_wall[1]+1] == 0):
            s_cells += 1

    return s_cells

for i in range(0, height):
        line = []
        for j in range(0, width):
                line.append(unvisited)
        maze.append(line)

starting_height = int(random.random()*height)
starting_width = int(random.random()*width)


if (starting_height == 0):
        starting_height += 1
if (starting_height == height-1):
        starting_height -= 1
if (starting_width == 0):
        starting_width += 1
if (starting_width == width-1):
        starting_width -= 1

maze[starting_height][starting_width] = cell
walls = []
walls.append([starting_height - 1, starting_width])
walls.append([starting_height, starting_width - 1])
walls.append([starting_height, starting_width + 1])
walls.append([starting_height + 1, starting_width])

maze[starting_height-1][starting_width] = 1
maze[starting_height][starting_width - 1] = 1
maze[starting_height][starting_width + 1] = 1
maze[starting_height + 1][starting_width] = 1

while (walls):
        rand_wall = walls[int(random.random()*len(walls))-1]

        if (rand_wall[1] != 0):
                if (maze[rand_wall[0]][rand_wall[1]-1] == 2 and maze[rand_wall[0]][rand_wall[1]+1] == 0):
                        s_cells = surroundingCells(rand_wall)

                        if (s_cells < 2):
                                maze[rand_wall[0]][rand_wall[1]] = 0

                                if (rand_wall[0] != 0):
                                        if (maze[rand_wall[0]-1][rand_wall[1]] != 0):
                                                maze[rand_wall[0]-1][rand_wall[1]] = 1
                                        if ([rand_wall[0]-1, rand_wall[1]] not in walls):
                                                walls.append([rand_wall[0]-1, rand_wall[1]])


                                if (rand_wall[0] != height-1):
                                        if (maze[rand_wall[0]+1][rand_wall[1]] != 0):
                                                maze[rand_wall[0]+1][rand_wall[1]] = 1
                                        if ([rand_wall[0]+1, rand_wall[1]] not in walls):
                                                walls.append([rand_wall[0]+1, rand_wall[1]])

                                if (rand_wall[1] != 0):	
                                        if (maze[rand_wall[0]][rand_wall[1]-1] != 0):
                                                maze[rand_wall[0]][rand_wall[1]-1] = 1
                                        if ([rand_wall[0], rand_wall[1]-1] not in walls):
                                                walls.append([rand_wall[0], rand_wall[1]-1])
                        

                        for wall in walls:
                                if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                                        walls.remove(wall)

                        continue

        if (rand_wall[0] != 0):
                if (maze[rand_wall[0]-1][rand_wall[1]] == 2 and maze[rand_wall[0]+1][rand_wall[1]] == 0):

                        s_cells = surroundingCells(rand_wall)
                        if (s_cells < 2):
                                maze[rand_wall[0]][rand_wall[1]] = 0

                                if (rand_wall[0] != 0):
                                        if (maze[rand_wall[0]-1][rand_wall[1]] != 0):
                                                maze[rand_wall[0]-1][rand_wall[1]] = 1
                                        if ([rand_wall[0]-1, rand_wall[1]] not in walls):
                                                walls.append([rand_wall[0]-1, rand_wall[1]])

                                if (rand_wall[1] != 0):
                                        if (maze[rand_wall[0]][rand_wall[1]-1] != 0):
                                                maze[rand_wall[0]][rand_wall[1]-1] = 1
                                        if ([rand_wall[0], rand_wall[1]-1] not in walls):
                                                walls.append([rand_wall[0], rand_wall[1]-1])

                                if (rand_wall[1] != width-1):
                                        if (maze[rand_wall[0]][rand_wall[1]+1] != 0):
                                                maze[rand_wall[0]][rand_wall[1]+1] = 1
                                        if ([rand_wall[0], rand_wall[1]+1] not in walls):
                                                walls.append([rand_wall[0], rand_wall[1]+1])

                        for wall in walls:
                                if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                                        walls.remove(wall)

                        continue

        if (rand_wall[0] != height-1):
                if (maze[rand_wall[0]+1][rand_wall[1]] == 2 and maze[rand_wall[0]-1][rand_wall[1]] == 0):

                        s_cells = surroundingCells(rand_wall)
                        if (s_cells < 2):
                                maze[rand_wall[0]][rand_wall[1]] = 0

                                if (rand_wall[0] != height-1):
                                        if (maze[rand_wall[0]+1][rand_wall[1]] != 0):
                                                maze[rand_wall[0]+1][rand_wall[1]] = 1
                                        if ([rand_wall[0]+1, rand_wall[1]] not in walls):
                                                walls.append([rand_wall[0]+1, rand_wall[1]])
                                if (rand_wall[1] != 0):
                                        if (maze[rand_wall[0]][rand_wall[1]-1] != 0):
                                                maze[rand_wall[0]][rand_wall[1]-1] = 1
                                        if ([rand_wall[0], rand_wall[1]-1] not in walls):
                                                walls.append([rand_wall[0], rand_wall[1]-1])
                                if (rand_wall[1] != width-1):
                                        if (maze[rand_wall[0]][rand_wall[1]+1] != 0):
                                                maze[rand_wall[0]][rand_wall[1]+1] = 1
                                        if ([rand_wall[0], rand_wall[1]+1] not in walls):
                                                walls.append([rand_wall[0], rand_wall[1]+1])

                        for wall in walls:
                                if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                                        walls.remove(wall)


                        continue

        if (rand_wall[1] != width-1):
                if (maze[rand_wall[0]][rand_wall[1]+1] == 2 and maze[rand_wall[0]][rand_wall[1]-1] == 0):

                        s_cells = surroundingCells(rand_wall)
                        if (s_cells < 2):
                                maze[rand_wall[0]][rand_wall[1]] = 0

                                if (rand_wall[1] != width-1):
                                        if (maze[rand_wall[0]][rand_wall[1]+1] != 0):
                                                maze[rand_wall[0]][rand_wall[1]+1] = 1
                                        if ([rand_wall[0], rand_wall[1]+1] not in walls):
                                                walls.append([rand_wall[0], rand_wall[1]+1])
                                if (rand_wall[0] != height-1):
                                        if (maze[rand_wall[0]+1][rand_wall[1]] != 0):
                                                maze[rand_wall[0]+1][rand_wall[1]] = 1
                                        if ([rand_wall[0]+1, rand_wall[1]] not in walls):
                                                walls.append([rand_wall[0]+1, rand_wall[1]])
                                if (rand_wall[0] != 0):	
                                        if (maze[rand_wall[0]-1][rand_wall[1]] != 0):
                                                maze[rand_wall[0]-1][rand_wall[1]] = 1
                                        if ([rand_wall[0]-1, rand_wall[1]] not in walls):
                                                walls.append([rand_wall[0]-1, rand_wall[1]])
                                                
                        for wall in walls:
                                if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                                        walls.remove(wall)

                        continue

        for wall in walls:
                if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                        walls.remove(wall)        

for i in range(0, height):
        for j in range(0, width):
                if (maze[i][j] == 2):
                        maze[i][j] = 1

for i in range(0, width):
        if (maze[1][i] == 0):
                maze[0][i] = 0
                break

for i in range(width-1, 0, -1):
        if (maze[height-2][i] == 0):
                maze[height-1][i] = 0
                break

if maze[1][1]: maze[1][1] = 0

MAZE = []

for line in maze[1:-1]:
    MAZE.append(line[1:-1])

center = (random.randint(0,width-2),random.randint(0,height-2))

while (maze[center[1]+1][center[0]+1]) or ((center[0],center[1]) == (1,1)):
    center = (random.randint(0,width-2),random.randint(0,height-2))

mazeArrayR = np.empty([height-2,width-2])
mazeArrayG = np.empty([height-2,width-2])
mazeArrayB = np.empty([height-2,width-2])

wall = 1
cell = 0
unvisited = 2

knownwalls = []
for y in range(len(MAZE)):
    for x in range(len(MAZE[0])):
        if MAZE[y][x]: knownwalls.append((x,y))

distmat = []
for i in range(len(MAZE)):
    distmat.append([0]*len(MAZE[0]))
updt = 0
for (x,y) in knownwalls:
    distmat[y][x] = (len(MAZE)*len(MAZE[0]))+1
    updt +=1

lwalls = len(knownwalls)

for i in range(len(MAZE)):
    knownwalls.append((-1,i))
    knownwalls.append((len(MAZE[0]),i))
for i in range(len(MAZE[0])):
    knownwalls.append((i,-1))
    knownwalls.append((i,len(MAZE)))
    
curdist = 0
curx, cury = center
distmat[cury][curx] = curdist
updt += 1
curdist = 1
nxs = []
mxs = [(curx,cury)]
k = 2


print('starting walls at' + str(strftime("%Y-%m-%d %H:%M:%S", gmtime())) + ' which is +4h')
print('starting gen with ' + str(lwalls) + ' walls')

while updt < (len(MAZE)*len(MAZE[0])) and k:
    if k == 1: k = 0
    lastupdt = updt
    nxs = mxs
    mxs = []
    for nx in nxs:
        curx = nx[0]
        cury = nx[1]
        for i in range(-2,2):
            x = (i%2)*i
            if i<0: y = i+1
            else: y = 1-x
            mx = curx + x
            my = cury + y
            if (not (mx,my) in knownwalls) and (distmat[my][mx] == 0) and ((mx,my) != center):
                mxs.append((mx,my))
                distmat[my][mx] = curdist
                updt +=1
    curdist +=1
    if lastupdt == updt and k == 2: k = 1
    
for y in range(len(MAZE)):
    for x in range(len(MAZE[0])):
        if distmat[y][x] == 0:
            distmat[y][x] = (len(MAZE)*len(MAZE[0])) + 1
            
distmat[center[1]][center[0]] = 0

gradRadius = curdist+7

for i in range(len(MAZE)):
    for j in range(len(MAZE[0])):
        if (MAZE[i][j] != wall):
            pointDist = distmat[i][j]
            pointHue = (pointDist/gradRadius)*360
            mazeArrayR[i][j] = pointHue
            mazeArrayG[i][j] = 1
            mazeArrayB[i][j] = 1
for i in range(len(MAZE)):
    for j in range(len(MAZE[0])):
        if(MAZE[i][j] == wall):
            mazeArrayR[i][j] = 0
            mazeArrayG[i][j] = 0
            mazeArrayB[i][j] = 0
        else:
            Cwilly = mazeArrayB[i][j]*mazeArrayG[i][j]
            pointHue = mazeArrayR[i][j]
            Xinbop = Cwilly*(1-abs(((pointHue/60)%2)-1))
            mincedMeat = mazeArrayB[i][j]-Cwilly
            pointHue = pointHue%360
            if 0 <= pointHue and pointHue < 60:
                colorPrime = [Cwilly,Xinbop,0]
            if 60 <= pointHue and pointHue < 120:
                colorPrime = [Xinbop,Cwilly,0]
            if 120 <= pointHue and pointHue < 180:
                colorPrime = [0,Cwilly,Xinbop]
            if 180 <= pointHue and pointHue < 240:
                colorPrime = [0,Xinbop,Cwilly]
            if 240 <= pointHue and pointHue < 300:
                colorPrime = [Xinbop,0,Cwilly]
            if 300 <= pointHue and pointHue < 360:
                colorPrime = [Cwilly,0,Xinbop]
            mazeArrayR[i][j] = (colorPrime[0]+mincedMeat)*255
            mazeArrayG[i][j] = (colorPrime[1]+mincedMeat)*255
            mazeArrayB[i][j] = (colorPrime[2]+mincedMeat)*255
            

mazeArrayR[center[1]][center[0]] = 255
mazeArrayG[center[1]][center[0]] = 255
mazeArrayB[center[1]][center[0]] = 255

mazeArray = np.zeros([height-2,width-2,3],dtype = np.uint8)
for i in range(len(MAZE)):
    for j in range(len(MAZE[0])):
                mazeArray[i][j][0] = mazeArrayR[i][j]
                mazeArray[i][j][1] = mazeArrayG[i][j]
                mazeArray[i][j][2] = mazeArrayB[i][j]
                


im = Image.fromarray(mazeArray)
im.show()
im.save('coolness.png')

print('finished at' + str(strftime("%Y-%m-%d %H:%M:%S", gmtime())) + ' which is +4h')
