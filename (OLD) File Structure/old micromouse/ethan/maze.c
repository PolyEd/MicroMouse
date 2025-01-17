#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define size 15000
int grid[size][size];
unsigned int colorgrid[size][size];
#define sig {'B','M'}
#define bytes 24
unsigned char *bmp() {
    uint32_t chars = 3*size*size + 54;
    
    unsigned char *map = (unsigned char *)malloc((chars)*sizeof(char));
    unsigned char *ret = map;
    //header and stuff
    {
    //signature (2 bytes)
    
    *(map++) = 'B';
    *(map++) = 'M';
    //file size (4 bytes)
    *(map++) = chars%(1<<8);
    *(map++) = (chars%(1<<16))>>8;
    *(map++) = (chars%(1<<24))>>16;
    *(map++) = 0;
    // reserved field (4 bytes)
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    //offset of pixel data (4 bytes)
    *(map++) = 54;
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;

    //header

    //header size
    *(map)++ = 40;
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    //width
    *(map++) = size%(1<<8);
    *(map++) = (size%(1<<16))>>8;
    *(map++) = (size%(1<<24))>>16;
    *(map++) = 0;
    //height
    *(map++) = size%(1<<8);
    *(map++) = (size%(1<<16))>>8;
    *(map++) = (size%(1<<24))>>16;
    *(map++) = 0;
    //reserved field
    *(map++) = 1;
    *(map++) = 0;
    //bits per pixel
    *(map++) = 24;
    *(map++) = 0;
    //compression method
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    //size of pixel data
    *(map++) = (chars-54)%(1<<8);
    *(map++) = ((chars-54)%(1<<16))>>8;
    *(map++) = ((chars-54)%(1<<24))>>16;
    *(map++) = 0;
    //horizontal resolution
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0b00110000;
    *(map++) = 0b10110001;
    //vertical resolution
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0b00110000;
    *(map++) = 0b10110001;
    //color palette info
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    //number of important colors
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    *(map++) = 0;
    }
    //pixel data
    for(int i = 0; i <size; i++) {
        for(int j = 0; j <size; j++) {
            if(grid[i][j] == 3) {
                if(colorgrid[i][j] < 200) *(map++) = 255-abs(((int)colorgrid[i][j])-50); else *(map++) = 255-abs((int)colorgrid[i][j]-350);
                *(map++) = 255-abs(150-(int)colorgrid[i][j]);
                if(colorgrid[i][j] > 100) *(map++) = 255-abs(((int)colorgrid[i][j])-250); else *(map++) = 255-abs((int)colorgrid[i][j]+50);
            } else {
                *(map++) = 0;
                *(map++) = 0;
                *(map++) = 0;
            }
        }
    }
    return ret;
};
struct node {
    int x;
    int y;
    struct node *next;
};
struct node *walls;
void addWall(int _x, int _y, struct node **n) {
    struct node *head = (struct node *)malloc(sizeof(struct node *));
    head->x = _x;
    head->y = _y;
    head->next = *n;
    *n = head;
};
void removeWall(struct node **n) {
    struct node *x = *n;
    *n = (*n)->next;
    free(x);
};
void shuffle(struct node **n, int c) {
    int f1 = rand() % c;
    int f2 = rand() % c;
    struct node *x1 = *n;
    struct node *x2 = *n;
    for(int i = 0; i<f1; i++) {
       x1 = x1->next; 
    }
    for(int i = 0; i<f2; i++) {
       x2 = x2->next; 
    }
    int carry = x1->x;
    x1->x = x2->x;
    x2->x = carry;
    carry = x1->y;
    x1->y = x2->y;
    x2->y = carry;
};
int main() {
    uint32_t totalWallsOver100 = (uint32_t)size*size/100*1.75;
    uint32_t counter = 0;
    int percent = 0;
    for(int i = 0; i < size; i++) {
        for(int j = 0; i < size; i++) {
            grid[i][j] = 0;
        }
    }
    {
    grid[size >> 1][size>>1] = 1;
    addWall((size>>1)-1,size>>1,&walls);
    addWall((size>>1)+1,size>>1,&walls);
    addWall(size>>1,(size>>1)-1,&walls);
    addWall(size>>1,(size>>1)+1,&walls);
    }
    while (walls != NULL)
    {
        struct node *add = NULL;
        if((
        (walls->x > 0 && (grid[(walls->x)-1][walls->y] == 1))+
        (walls->x < size-1 && (grid[(walls->x)+1][walls->y] == 1))+
        (walls->y > 0 && (grid[walls->x][(walls->y-1)] == 1))+
        (walls->y < size-1 && (grid[walls->x][(walls->y)+1] == 1))
        ) < 2
        ) {
            int c = 0;
            grid[walls->x][walls->y] = 1;
            if(walls->x > 0 && (grid[(walls->x)-1][walls->y] == 0)) {
                addWall((walls->x)-1,walls->y,&add);
                c += 1;
            }
            if(walls->x < size-1 && (grid[(walls->x)+1][walls->y] == 0)) {
                addWall((walls->x)+1,walls->y,&add);
                c += 1;
            }
            if(walls->y > 0 && (grid[walls->x][(walls->y)-1] == 0)) {
                addWall(walls->x,(walls->y)-1,&add);
                c += 1;
            }
            if(walls->y < size - 1 && (grid[walls->x][(walls->y)+1] == 0)) {
                addWall(walls->x,(walls->y)+1,&add);
                c += 1;
            }
            if(c>1){
                for(int i = 0; i<3; i++) {
                    shuffle(&add,c);
                }
            }
        }
        removeWall(&walls);
        if(counter == totalWallsOver100) {
            counter = 0;
            percent += 1;
            printf("%d%%\n",percent);
        } else {
            counter += 1; 
        }

        if(add != NULL) {
            struct node *head = add;;
            while(add->next != NULL) {
                add = add->next;
            };
            add->next = walls;
            walls = head;
        }
    };
    printf("done\n");
    struct node *cells = NULL;
    addWall(size>>1,size>>1,&cells);
    unsigned int time = 0;
    while(cells != NULL){
        time += 1;
        time %= 300;
        struct node *next = NULL;
        while(cells != NULL) {
        colorgrid[cells->x][cells->y] = time;
        grid[cells->x][cells->y] = 3;
        if(cells->x > 0 && (grid[(cells->x)-1][cells->y] == 1)) {
            addWall((cells->x)-1,cells->y,&next);
        }
        if(cells->x < size - 1 && (grid[(cells->x)+1][cells->y] == 1)) {
            addWall(cells->x+1,cells->y,&next);
        }
        if(cells->y > 0 && (grid[cells->x][(cells->y)-1] == 1)) {
            addWall(cells->x,cells->y-1,&next);
        }
        if(cells->y < size - 1 && (grid[cells->x][(cells->y)+1] == 1)) {
            addWall(cells->x,cells->y+1,&next);
        }
        removeWall(&cells);
        }
        cells = next;
    }
    printf("done\n");
    char name[] = "bit0.bmp";
    unsigned char *bitmap = bmp();
    FILE *file;
    file = fopen(name,"w+");
    uint32_t chars = 3*size*size+54;
    for(int i = 0; i < chars; i++) {
        fputc(*(bitmap++),file);
    }
    fclose(file);
    return 0;
}
