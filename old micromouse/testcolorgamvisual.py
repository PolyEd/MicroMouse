import pygame
import time
import random
from math import *

pygame.init()

screen = pygame.display.set_mode((400,400))

font = pygame.font.SysFont('arialunicode',22)

play = True

r = random.randint(0,255)
g = random.randint(0,255)
b = random.randint(0,255)

col = pygame.Color(r,g,b)
gam = 0

text = font.render(str(str(r) + ', ' + str(g) + ', ' + str(b) + ', ' + str(trunc(float(gam*100))/100)),True,(255,255,255))

sdown = False
scur = False

while 1:
    for event in pygame.event.get():
        if event.type == pygame.QUIT: exit()
    keys = pygame.key.get_pressed()
    scur = sdown
    if keys[pygame.K_SPACE]:
        sdown = True
    else: sdown = False; scur = False
    if sdown is not scur: play = not play
    if play: gam += 0.01
    if gam > 2:
        gam = 0
        r = random.randint(0,255)
        g = random.randint(0,255)
        b = random.randint(0,255)

        col = pygame.Color(r,g,b)
    if gam<1:
        screen.fill(col.correct_gamma(gam))
        text = font.render(str(str(r) + ', ' + str(g) + ', ' + str(b) + ', ' + str(trunc(float(gam*100))/100)),True,(255,255,255))
    if gam>1:
        screen.fill(col.correct_gamma(2-gam))
        text = font.render(str(str(r) + ', ' + str(g) + ', ' + str(b) + ', ' + str(trunc(float((2-gam)*100))/100)),True,(255,255,255))
    screen.blit(text,(200-text.get_width()/2,200-text.get_height()/2))
    
    pygame.display.update()
    time.sleep(0.005)
