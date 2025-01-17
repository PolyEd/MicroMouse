#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#define mazeSize 24
#define start (point){12,12}
// bmp drawer
void render(int mValues[mazeSize][mazeSize],
bool wSide[mazeSize-1][mazeSize],
bool wUpDown[mazeSize][mazeSize-1], char name[]) {
    FILE *file = fopen(name,"w+");
    int chars = 47*48*3 + 54;
    //header and stuff
    {
    //signature (2 bytes)
    
    fputc('B',file);
    fputc('M',file);
    //file size (4 bytes)
    fputc(chars%(1<<8),file);
    fputc((chars%(1<<16))>>8,file);
    fputc((chars%(1<<24))>>16,file);
    fputc(0,file);
    // reserved field (4 bytes)
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    //offset of pixel data (4 bytes)
    fputc(54,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);

    //header

    //header size
    fputc(40,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    //width
    fputc(47,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    //height
    fputc(47,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    //reserved field
    fputc(1,file);
    fputc(0,file);
    //bits per pixel
    fputc(24,file);
    fputc(0,file);
    //compression method
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    //size of pixel data
    fputc((chars-54)%(1<<8),file);
    fputc(((chars-54)%(1<<16))>>8,file);
    fputc(((chars-54)%(1<<24))>>16,file);
    fputc(0,file);
    //horizontal resolution
    fputc(0,file);
    fputc(0,file);
    fputc(0b00110000,file);
    fputc(0b10110001,file);
    //vertical resolution
    fputc(0,file);
    fputc(0,file);
    fputc(0b00110000,file);
    fputc(0b10110001,file);
    //color palette info
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    //number of important colors
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    fputc(0,file);
    }
    //pixel data
    for(int i = 0; i < 47; i++) {
        if(i%2 == 0) {
            for(int j = 0; j < 47; j++) {
                if(j%2 == 0) {
                    if(mValues[i>>1][j>>1] == -1) {
                        fputc(0,file); fputc(0,file); fputc(255,file);
                    } else {
                        fputc(255,file); fputc(255,file); fputc(255,file);   
                    }
                } else {
                    if(wUpDown[i>>1][(j-1)>>1]) {
                        fputc(255,file); fputc(0,file); fputc(0,file);
                    } else {
                        fputc(255,file); fputc(255,file); fputc(255,file);
                    }
                }
            }
        } else {
            for(int j = 0; j < 47; j++) {
                if(j%2 == 0) {
                    if(wSide[(i-1)>>1][j>>1]) {
                        fputc(255,file); fputc(0,file); fputc(0,file);
                    } else {
                        fputc(255,file); fputc(255,file); fputc(255,file);
                    }
                    
                } else {
                    fputc(255,file); fputc(0,file); fputc(0,file);
                }
            }
        }
        //padding
        fputc(0,file); fputc(0,file); fputc(0,file);
    }
    fclose(file);
};
// actual code
typedef struct point {
    int x;
    int y;
} point;
typedef struct link {
    struct link *next;
    point pos;
} link;
void append(link **listToAppendTo, point p) {
    link *l = (link *)malloc(sizeof(link));
    l->next = *listToAppendTo;
    l->pos = p;
    *listToAppendTo = l;
};
bool iter(link **l) {
    if((*l)->next == NULL) {
        free(*l);
        return false;
    }
    link *next = (*l)->next;
    free(*l);
    *l = next;
    return true;
};
int mazeValues[mazeSize][mazeSize];
bool wallSide[mazeSize-1][mazeSize];
bool wallUpDown[mazeSize][mazeSize-1];
int main() {
    for(int i = 0; i < mazeSize; i++) {
        for(int j = 0; j < mazeSize; j++) {
            mazeValues[i][j] = 0;
        }
    }
    for(int i = 0; i < mazeSize; i++) {
        for(int j = 0; j < mazeSize-1; j++) {
            wallUpDown[i][j] = (rand()%3 == 0);
        }
    }
    for(int i = 0; i < mazeSize-1; i++) {
        for(int j = 0; j < mazeSize; j++) {
            wallSide[i][j] = (rand()%3 == 0);
        }
    }
    link *l = NULL;
    append(&l,start);
    int currentStep = 2;
    mazeValues[l->pos.x][l->pos.y] = 1;
    do {
        link *nextL = NULL;
        link *copy = l;
        do {
            
            if(l->pos.x > 0 && !wallSide[l->pos.x-1][l->pos.y] && mazeValues[l->pos.x-1][l->pos.y] == 0) {
                append(&nextL,(point){l->pos.x-1,l->pos.y});
                mazeValues[l->pos.x-1][l->pos.y] = currentStep;
            }
            if(l->pos.x < mazeSize-1 && !wallSide[l->pos.x][l->pos.y] && mazeValues[l->pos.x+1][l->pos.y] == 0) {
                append(&nextL,(point){l->pos.x+1,l->pos.y});
                mazeValues[l->pos.x+1][l->pos.y] = currentStep;
            }
            if(l->pos.y > 0 && !wallUpDown[l->pos.x][l->pos.y-1] && mazeValues[l->pos.x][l->pos.y-1] == 0) {
                append(&nextL,(point){l->pos.x,l->pos.y-1});
                mazeValues[l->pos.x][l->pos.y-1] = currentStep;
            }
            if(l->pos.y < mazeSize-1 && !wallUpDown[l->pos.x][l->pos.y] && mazeValues[l->pos.x][l->pos.y+1] == 0) {
                append(&nextL,(point){l->pos.x,l->pos.y+1});
                mazeValues[l->pos.x][l->pos.y+1] = currentStep;
            }
        } while(iter(&l));
        currentStep++;
        l = nextL;
    } while(mazeValues[0][0] == 0 && l != NULL);
    point p = (point){0, 0};
    if(l == NULL) {
        return 0;
    }
    while(mazeValues[start.x][start.y] != -1) {
        int C = mazeValues[p.x][p.y]-1;
        mazeValues[p.x][p.y] = -1;
        if(p.y > 0 && !wallUpDown[p.x][p.y-1]&&mazeValues[p.x][p.y-1]== C) {
            p.y--;
        } else if(p.y < mazeSize -1 && !wallUpDown[p.x][p.y] && mazeValues[p.x][p.y+1]==C) {
            p.y++;
        } else if(p.x > 0 && !wallSide[p.x-1][p.y] && mazeValues[p.x-1][p.y]==C) {
            p.x--;
        } else {
            p.x++;
        }
        printf("%d,%d,%d\n",p.x,p.y,C);
        
    }
    render(mazeValues,wallSide,wallUpDown,"r.bmp");
    return 0;
};
