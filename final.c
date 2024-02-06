/*
 *  MTB RPG
 *
 *  
 */
#include "CSCIx229.h"

int fov=80;          //  Field of view (for perspective) 50 default
double asp=1;        //  Aspect ratio
double dim=1;      //  Size of world
double moment = 0;     // time keeper for moving objects

int th= -0;         // Azimuth of view angle
int ph= -15;          // Elevation of view angle
double px= 0;        // X position of the player
double py= 0;        // Y position of the player
double pz= 0;        // Z position of the player
double Cx = 0;       // Eye x
double Cy = 0;       // Eye y
double Cz = 0;       // Eye z
double dist = 2;     // Third person zoom distance

const int length_bound = 5;   // world length
const int width_bound = 3;    // world Width
const int res = 15;           // Resolution of the world map. "items" per bounds unit

int mode = 0;        //  Viewing Mode 0: First Person 1: Projection View 2: Orthographic
int world = 0;       //  Display the Various Worlds
int item = 1;        //  Items in test mode
int axes = 0;        //  Display axes 

// Light values
int light     =   1;  // Lighting on off
int pause     =   1;  // Lighting pause switch
double pausetime = 0; // variable to hold elapsed time while paused. keeps light in sync
int one       =   1;  // Unit value
int distance  =   2;  // Light distance
int inc       =  10;  // Ball increment
int smooth    =   1;  // Smooth/Flat shading
int local     =   1;  // Local Viewer Model
int emission  =   100;  // Emission intensity (%)
int ambient   =  30;  // Ambient intensity (%)
int diffuse   =  40;  // Diffuse intensity (%)
int specular  =   0;  // Specular intensity (%)
int shininess =   0;  // Shininess (power of two)
float shiny   =   1;  // Shininess (value)
int zh        =  90;  // Light azimuth
float ylight  =   -1;  // Elevation of light

unsigned int texture[20];  //  Texture names

int win_height = 1000; // window height
int win_width = 1200;  // window width

struct tree {
   float x;
   float y;
   float z;
   float height;
   float width;
   float scale;
   float shade;
};

struct rock {
   int true;
   float x;
   float y;
   float z;
   float th;
   float s;
   float stretch;
};

const static int MAX_ROCK_PATCH = 4;

struct rockpatch {
   struct rock patch[MAX_ROCK_PATCH][MAX_ROCK_PATCH];
};

struct grass {
   int true;
   float x;
   float y;
   float z;
   float s;
   float shade;
};

const static int MAX_GRASS_PATCH = 30;

struct grasspatch {
   int true;
   struct grass patch[MAX_GRASS_PATCH][MAX_GRASS_PATCH];
};

struct bridge {
   int true;
   float startz;
   float startx;
   float endz;
   float endx;
   double starth;
   double endh;
   int heading;
   float alpha;
   int steps;
   float dist;
};

struct tree trees[width_bound*res][length_bound*res];
struct rockpatch rocks[width_bound*res][length_bound*res];
struct grasspatch grass[width_bound*res][length_bound*res];
const int MAX_BRIDGES = 10;
struct bridge bridges[MAX_BRIDGES];


static double world_map[width_bound*res][length_bound*res][2];
static float xpath[length_bound*res];
const int MAX_RIVER = 2*length_bound*res;
// const int MAX_RIVER = 10;
static float riverpath[MAX_RIVER][2];

// World Features
#define OOB -1
#define EMPTY 0
#define TREE 1
#define ROCK 2
#define RIVER 3
#define TRAIL 5
#define BRIDGE 6
#define GRASS 7

float white[3] = {1,1,1};
float dirt[3] = {0.45, 0.40, 0.39};

/*
 *  Draw vertex in polar coordinates
 */
static void Vertex(double th,double ph)
{
   glVertex3d(Sin(th)*Cos(ph) , Sin(ph) , Cos(th)*Cos(ph));
}

float findY(float x, float z) {
   x = (x)*(float)res;
   z = (z)*(float)res;
   if(x < 0 || x > width_bound*res || z < 0 || z > length_bound*res) {
      printf("ERROR: findY: X or Z Position Out Of Bounds");
      exit(0);
   }

   int x0 = floor(x);
   int x1 = ceil(x);
   int z0 = floor(z);
   int z1 = ceil(z);

   double y0 = world_map[x0][z0][0];
   double yx = world_map[x1][z0][0];
   double yz = world_map[x0][z1][0];
   double y1 = world_map[x1][z1][0];

   double xr = (x-(double)x0);
   double zr = (z-(double)z0);

   // double mag0 = sqrt(pow(x0,2)+pow(z0,2));
   // double magxz = sqrt(pow(x,2)+pow(z,2));
   double r = sqrt(pow(x-x0,2)+pow(z-z0,2));

   double y;
   
   if(r >= sqrt(2)/2+res) {
      xr = (x-(double)x1);
      zr = (z-(double)z1);
      y = y1 + ((.5*xr)*(yx-y1)) + ((.5*zr)*(yx-y1));
   }
   else {
      y = y0 + (xr*(yx-y0)) + (zr*(yz-y0));
   }

   // glWindowPos2i(5,120);
   // Print("Y0: %f, YX: %f, YZ: %f, Y1: %f",y0,yx,yz,y1);
   // -world_map[width_bound*res/2][length_bound*res/2][0]
   // *(2*dim/(double)res)
   // -((double)width_bound/2.0)
   return ((y)/(double)res)-(double)width_bound/2.0;
}

/*
 *  Draw a ball
 *     at (x,y,z)
 *     radius (r)
 */
static void ball(double x,double y,double z,double r)
{
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glScaled(r,r,r);
   //  White ball with yellow specular
   float yellow[]   = {1.0,1.0,0.0,1.0};
   float Emission[] = {0.0,0.0,0.01*emission,1.0};
   glColor3f(1,1,1);
   glMaterialf(GL_FRONT,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT,GL_SPECULAR,yellow);
   glMaterialfv(GL_FRONT,GL_EMISSION,Emission);
   //  Bands of latitude
   for (int ph=-90;ph<90;ph+=inc)
   {
      glBegin(GL_QUAD_STRIP);
      for (int th=0;th<=360;th+=2*inc)
      {
         Vertex(th,ph);
         Vertex(th,ph+inc);
      }
      glEnd();
   }
   //  Undo transofrmations
   glPopMatrix();
}

void surfacenormal(double *normals,int x, int z) {
   double xzy = world_map[x][z][0];

   double n1[3] = cross(-1,   +0,   fabs(world_map[x-1][z][0]-xzy),     fabs(world_map[x][z+1][0]-xzy),     +0,   +1);
   double sumn1 = sqrt(pow(n1[0],2)+pow(n1[1],2)+pow(n1[2],2));
   n1[0] = n1[0]/sumn1;
   n1[1] = n1[1]/sumn1;
   n1[2] = n1[2]/sumn1;

   // double n2[3] = {0,0,0};
   double n2[3] = cross(-1,   -1,   fabs(world_map[x-1][z-1][0]-xzy),   fabs(world_map[x-1][z][0]-xzy),     -1,   +0);
   double sumn2 = sqrt(pow(n2[0],2)+pow(n2[1],2)+pow(n2[2],2));
   n2[0] = n2[0]/sumn2;
   n2[1] = n2[1]/sumn2;
   n2[2] = n2[2]/sumn2;

   // double n3[3] = {0,0,0};
   double n3[3] = cross(+0,   -1,   fabs(world_map[x][z-1][0]-xzy),     fabs(world_map[x-1][z-1][0]-xzy),   -1,   -1);
   double sumn3 = sqrt(pow(n3[0],2)+pow(n3[1],2)+pow(n3[2],2));
   n3[0] = n3[0]/sumn3;
   n3[1] = n3[1]/sumn3;
   n3[2] = n3[2]/sumn3;

   // double n4[3] = {0,0,0};
   double n4[3] = cross(+1,   +0,   fabs(world_map[x+1][z][0]-xzy),     fabs(world_map[x][z-1][0]-xzy),     +0,   -1);
   double sumn4 = sqrt(pow(n4[0],2)+pow(n4[1],2)+pow(n4[2],2));
   n4[0] = n4[0]/sumn4;
   n4[1] = n4[1]/sumn4;
   n4[2] = n4[2]/sumn4;

   // double n5[3] = {0,0,0};
   double n5[3] = cross(+1,   +1,   fabs(world_map[x+1][z+1][0]-xzy),   fabs(world_map[x+1][z][0]-xzy),     +1,   +0);
   double sumn5 = sqrt(pow(n5[0],2)+pow(n5[1],2)+pow(n5[2],2));
   n5[0] = n5[0]/sumn5;
   n5[1] = n5[1]/sumn5;
   n5[2] = n5[2]/sumn5;

   // double n6[3] = {0,0,0};
   double n6[3] = cross(+0,   +1,   fabs(world_map[x][z+1][0]-xzy),     fabs(world_map[x+1][z+1][0]-xzy),   +1,   +1);
   double sumn6 = sqrt(pow(n6[0],2)+pow(n6[1],2)+pow(n6[2],2));
   n6[0] = n6[0]/sumn6;
   n6[1] = n6[1]/sumn6;
   n6[2] = n6[2]/sumn6;

   normals[0] = 2*((+n1[0]+n2[0]+n3[0]+n4[0]+n5[0]+n6[0])/6.0);
   normals[1] = .5*((+n1[1]+n2[1]+n3[1]+n4[1]+n5[1]+n6[1])/6.0);
   normals[2] = -2*((+n1[2]+n2[2]+n3[2]+n4[2]+n5[2]+n6[2])/6.0);
}

static void surface(float color[]) {
   double dres = (double)res;
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,.01,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[0]);
   glBegin(GL_TRIANGLES);
   // Variables for texture stretching across multiple panels
   int panels = 2;
   double step = 1.0;
   // Initialization for variables to help convert grid coordinates to real coordinates
   double y = 0;
   double newj = 0;
   double newi = 0;
   double normals[3] = {0,0,0};
   for(int j = 1; j < length_bound*res-1; j+=1) {
      for(int i = 1; i < width_bound*res-1; i+=0) {
         // for(int block = 0; block <= 1; block+=2) {
            
            y = world_map[i][j][0];
            newi = (double)i/dres;
            newj = (double)j/dres;
            surfacenormal(normals,i,j);
            glNormal3f(normals[0],normals[1],normals[2]);
            glColor3f(color[0],color[1],color[2]);
            glTexCoord2f((double)(j % panels)*step,(double)(i % panels)*step); 
            glVertex3d(newi,y/dres,newj);

            j+=1;
            i+=1;
            y = world_map[i][j][0];
            newi = (double)i/dres;
            newj = (double)j/dres;
            surfacenormal(normals,i,j);
            glNormal3f(normals[0],normals[1],normals[2]);
            glColor3f(color[0],color[1],color[2]);
            glTexCoord2f((double)(j % panels)*step,(double)(i % panels)*step); 
            glVertex3d(newi,y/dres,newj);
            
            i -= 1;
            y = world_map[i][j][0];
            newi = (double)i/dres;
            newj = (double)j/dres;
            surfacenormal(normals,i,j);
            glNormal3f(normals[0],normals[1],normals[2]);
            glColor3f(color[0],color[1],color[2]);
            glTexCoord2f((double)(j % panels)*step,(double)(i % panels)*step*2); 
            glVertex3d(newi,y/dres,newj);

            j-=1;
            y = world_map[i][j][0];
            newi = (double)i/dres;
            newj = (double)j/dres;
            surfacenormal(normals,i,j);
            glNormal3f(normals[0],normals[1],normals[2]);
            glColor3f(color[0],color[1],color[2]);
            glTexCoord2f((double)(j % panels)*step,(double)(i % panels)*step*2); 
            glVertex3d(newi,y/dres,newj);

            i+=1; 
            y = world_map[i][j][0];
            newi = (double)i/dres;
            newj = (double)j/dres;
            surfacenormal(normals,i,j);
            glNormal3f(normals[0],normals[1],normals[2]);
            glColor3f(color[0],color[1],color[2]);
            glTexCoord2f((double)(j % panels)*step,(double)(i % panels)*step); 
            glVertex3d(newi,y/dres,newj);
            
            j += 1;
            y = world_map[i][j][0];
            newi = (double)i/dres;
            newj = (double)j/dres;
            surfacenormal(normals,i,j);
            glNormal3f(normals[0],normals[1],normals[2]);
            glColor3f(color[0],color[1],color[2]);
            glTexCoord2f((double)(j % panels)*step,(double)(i % panels)*step*2); 
            glVertex3d(newi,y/dres,newj);
            
            j-=1;
      }
         // // glNormal3f(newi,fabs(y),newj);
         // glColor3f(color[0],color[1],color[2]);
         // glTexCoord2f((double)(j % panels)*step,(double)(0 % panels)*step); 
         // glVertex3d(newi,y/dres,newj);

         // y = world_map[0][j][0];
         // newi = (double)0/dres;
         // newj = (double)j/dres;
         // // glNormal3f(newi,fabs(y),newj);
         // glColor3f(color[0],color[1],color[2]);
         // glTexCoord2f((double)(j % panels)*step,(double)(0 % panels)*step); 
         // glVertex3d(newi,y/dres,newj);
   }
   glEnd();
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
   glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

static void skycube(float x,float y,float z,float w,float th,float color[]) {
   //  Save transformation
   glPushMatrix();
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glRotated(th,0,1,0);
   glScaled(w,w,w);
   //  Cube
   glColor3f(color[0],color[1],color[2]);
   //  Front
   glBindTexture(GL_TEXTURE_2D,texture[9]);
   double boxlim = .998;
   glBegin(GL_QUADS);
   glNormal3f(0,0,-1); glTexCoord2f(boxlim,0); glVertex3f(-1,-1,+1);
   glNormal3f(0,0,-1); glTexCoord2f(0,0); glVertex3f(+1,-1,+1);
   glNormal3f(0,0,-1); glTexCoord2f(0,boxlim); glVertex3f(+1,+1,+1);
   glNormal3f(0,0,-1); glTexCoord2f(boxlim,boxlim); glVertex3f(-1,+1,+1);
   glEnd();
   //  Right
   glBindTexture(GL_TEXTURE_2D,texture[10]);
   glBegin(GL_QUADS);
   glNormal3f(+1,0,0); glTexCoord2f(1,0); glVertex3f(-1,-1,-1);
   glNormal3f(+1,0,0); glTexCoord2f(0,0); glVertex3f(-1,-1,+1);
   glNormal3f(+1,0,0); glTexCoord2f(0,boxlim); glVertex3f(-1,+1,+1);
   glNormal3f(+1,0,0); glTexCoord2f(boxlim,boxlim); glVertex3f(-1,+1,-1);
   glEnd();
   //  Back
   glBindTexture(GL_TEXTURE_2D,texture[11]);
   glBegin(GL_QUADS);
   glNormal3f(-1,+1,0); glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
   glNormal3f(+1,+1,0); glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   glNormal3f(+1,+1,0); glTexCoord2f(0,boxlim); glVertex3f(-1,+1,-1);
   glNormal3f(-1,+1,0); glTexCoord2f(boxlim,boxlim); glVertex3f(+1,+1,-1);
   glEnd();
   //  Left
   glBindTexture(GL_TEXTURE_2D,texture[12]);
   glBegin(GL_QUADS);
   glNormal3f(-1,+1,0); glTexCoord2f(1,0); glVertex3f(+1,-1,+1);
   glNormal3f(-1,+1,0); glTexCoord2f(0,0); glVertex3f(+1,-1,-1);
   glNormal3f(-1,+1,0); glTexCoord2f(0,boxlim); glVertex3f(+1,+1,-1);
   glNormal3f(-1,+1,0); glTexCoord2f(boxlim,boxlim); glVertex3f(+1,+1,+1);
   glEnd();
   //  Top
   glBindTexture(GL_TEXTURE_2D,texture[13]);
   glBegin(GL_QUADS);
   glNormal3f(+1,-1,0); glTexCoord2f(1,0); glVertex3f(-1,+1,+1);
   glNormal3f(-1,-1,0); glTexCoord2f(0,0); glVertex3f(+1,+1,+1);
   glNormal3f(-1,-1,0); glTexCoord2f(0,boxlim); glVertex3f(+1,+1,-1);
   glNormal3f(+1,-1,0); glTexCoord2f(boxlim,boxlim); glVertex3f(-1,+1,-1);
   glEnd();
   //  Bottom
   glBindTexture(GL_TEXTURE_2D,texture[14]);
   glBegin(GL_QUADS);
   glNormal3f(0,+1,0); glTexCoord2f(1,0); glVertex3f(-1,-1,-1);
   glNormal3f(0,+1,0); glTexCoord2f(0,0); glVertex3f(+1,-1,-1);
   glNormal3f(0,+1,0); glTexCoord2f(0,boxlim); glVertex3f(+1,-1,+1);
   glNormal3f(0,+1,0); glTexCoord2f(boxlim,boxlim); glVertex3f(-1,-1,+1);
   glEnd();
   glDisable(GL_TEXTURE_2D);
   //  Undo transofrmations
   glPopMatrix();
}

static void drawtree(float x, float y, float z, float height, float width, float scale, float shade) {
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,.01,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);

   glTranslatef(x,y-.1,z);
   glScaled(scale/(double)res,scale/(double)res,scale/(double)res);

   glBindTexture(GL_TEXTURE_2D,texture[2]);
   // ANIMATION VARIABLES
   double speed = 1;
   double factor = moment*fmod(x * z, 2)*speed;
   double sway = (Sin(factor) - Sin(factor/2) + Sin(factor/4) - Sin(factor/8))/20;

   // tree
   height = height * 8;
   double base = width/2;
   double b[3] = {.3,.3,.3};
   double mod = 0;
   int sides = 8;
   int angle = 360/sides;
   int phi = 0;
   int mu = 0;
   glBegin(GL_TRIANGLES);
   // trunk
   for(int i = 0; i<sides;i++) {
      phi = (i) * angle;
      mu = (i+1) * angle;
      // mod = Cos(phi)*.06;
      // trunk
      glColor3f(b[0]+mod,b[1]+mod,b[0]+mod);
      glNormal3f(Cos(phi),1,Sin(phi)); glTexCoord2f(2*(double)phi/180.0,0);     glVertex3f(Cos(phi)*base,0,Sin(phi)*base);
      glNormal3f(Cos(mu),1,Sin(mu));   glTexCoord2f(2*(double)mu/180.0,0.0);   glVertex3f(Cos(mu)*base,0, Sin(mu)*base);
      glNormal3f(0,1,0);               glTexCoord2f((double)(mu-phi)/360.0,20);     glVertex3f(sway,height, 0);
   }
   glEnd();

   glBindTexture(GL_TEXTURE_2D,texture[1]);
   // leaves
   const int N = 18;
   const unsigned char index[] = 
   {
      1,0,3,   
      0,1,2,   
      1,2,4,
      1,3,4,
      0,4,3,
      0,2,4,
   };
   double leaf_height = height/10;
   float xyz[] =
   {
      0, height, 0, 
      width, height - leaf_height, 0,
      (width*3/5)*Cos(-angle), height - leaf_height*4/5, (width*3/5)*Sin(-angle),
      (width*3/5)*Cos(+angle), height - leaf_height*4/5, (width*3/5)*Sin(+angle),
      0, height - leaf_height*6/7, 0,      
   };
   float rep = 15;
   float txyz[] =
   {
      0,0, // Top of the tree
      rep*.5,rep*.6, // Furthest point
      0,rep*.5, //
      rep*1,rep*.5,
      rep*.5,rep*1,
   };
   float color[3] = {.5,.5,.5};
   const float rgb[] = 
   {
      color[0]+shade,color[1]+shade,color[0]+shade,
      color[0]-.12+shade,color[1]-.26+shade,color[0]-.13+shade,
      color[0]+shade,color[1]+shade,color[0]+shade,
      color[0]-.19+shade,color[1]-.22+shade,color[0]-.19+shade,
      color[0]-.22+shade,color[1]-.51+shade,color[0]-.29+shade,
   };
   // const float rgb[] = 
   // {
   //    1,1,1,
   //    1,1,1,
   //    1,1,1,
   //    1,1,1,
   //    1,1,1,
   // };
   const float norms[] =
   {
      0,1,0,
      1, 1 , 0,
      1*Cos(-angle), 6/5, 1*Sin(-angle),
      1*Cos(+angle), 6/5, 1*Sin(+angle),
      0, 6/7, 0, 
   };
   //  Define vertexes
   glVertexPointer(3,GL_FLOAT,0,xyz);
   glEnableClientState(GL_VERTEX_ARRAY);
   //  Define colors for each vertex
   glColorPointer(3,GL_FLOAT,0,rgb);
   glEnableClientState(GL_COLOR_ARRAY);
   // Define textures
   glTexCoordPointer(2,GL_FLOAT,0,txyz);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   // Define normals
   glNormalPointer(GL_FLOAT,0,norms);
   glEnableClientState(GL_NORMAL_ARRAY);
   glTranslatef(sway,leaf_height,0);
   sway = sway/60;
   for(int j = 0; j<(height/leaf_height); j++) {
      glScalef(1.15,1.15,1.15);
      glTranslatef(sway,-leaf_height*1.4,0);
      glRotatef(20,0,1,0);
      for(int i = 0; i<sides; i++) {
         double lth = i * angle;
         glRotatef(lth,0,1,0);
         glDrawElements(GL_TRIANGLES,N,GL_UNSIGNED_BYTE,index);
      }
   }
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
   //  Disable vertex array
   glDisableClientState(GL_VERTEX_ARRAY);
   //  Disable color array
   glDisableClientState(GL_COLOR_ARRAY);
   // Disable texture array
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   // Disable normal array
   glDisableClientState(GL_NORMAL_ARRAY);
}

static void drawrock(float x,float y,float z,float th,float s,int stretch)
{
   //  Set specular color to white
   s = s/15;
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,.01,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,1);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);

   glBindTexture(GL_TEXTURE_2D,texture[3]);

   //  Vertex index list
   const int N=60;
   const unsigned char index[] =
      {
       2, 1, 0,    3, 2, 0,    4, 3, 0,    5, 4, 0,    1, 5, 0,
      11, 6, 7,   11, 7, 8,   11, 8, 9,   11, 9,10,   11,10, 6,
       1, 2, 6,    2, 3, 7,    3, 4, 8,    4, 5, 9,    5, 1,10,
       2, 7, 6,    3, 8, 7,    4, 9, 8,    5,10, 9,    1, 6,10,
      };
   //  Vertex coordinates
   const float xyz[] =
      {
       0.000, 0.000, 1.000, //1
       stretch*0.894, 0.000, 0.447, //2
       0.276, 0.851, 0.447, //3
      -stretch*0.724, 0.526, 0.447, //4
      -0.724,-0.526, 0.447, //5
       stretch*0.276,-0.851, 0.447, //6
       0.724, 0.526,-0.447*stretch, //7
      -0.276, 0.851,-0.447*stretch, //8
      -0.894, 0.000,-0.447, //9
      -0.276,-0.851,-0.447*stretch, //10
       stretch*0.724,-0.526,-0.447, //11
       0.000, 0.000,-1.000  //12
      };
      const float txyz[] =
      {
       0, 0, //1
       1, 0, //2
       1, 1, //3
       0, 1, //4
       1, 0, //5
       0, 1, //6
       0, -1, //7
       -1, 0, //8
       -1, -1, //9
       0, -1, //10
       -1, 0, //11
       0, 0, //12
      };
   //  Vertex colors
   const float rgb[] =
      {
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      .38,.38,.38,
      };
   const float norms[] =
   {
      0.000, 0.000, 1.000, //1
       stretch*0.894, 0.000, 0.447, //2
       0.276, 0.851, 0.447, //3
      -stretch*0.724, 0.526, 0.447, //4
      -0.724,-0.526, 0.447, //5
       stretch*0.276,-0.851, 0.447, //6
       0.724, 0.526,-0.447*stretch, //7
      -0.276, 0.851,-0.447*stretch, //8
      -0.894, 0.000,-0.447, //9
      -0.276,-0.851,-0.447*stretch, //10
       stretch*0.724,-0.526,-0.447, //11
       0.000, 0.000,-1.000  //12
   };
   //  Define vertexes
   glVertexPointer(3,GL_FLOAT,0,xyz);
   glEnableClientState(GL_VERTEX_ARRAY);
   //  Define colors for each vertex
   glColorPointer(3,GL_FLOAT,0,rgb);
   glEnableClientState(GL_COLOR_ARRAY);
   // Define textures
   glTexCoordPointer(2,GL_FLOAT,0,txyz);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   // Define normals
   glNormalPointer(GL_FLOAT,0,norms);
   glEnableClientState(GL_NORMAL_ARRAY);
   //  Draw icosahedron
   glPushMatrix();
   glTranslated(x,y,z);
   glRotated(th,0,1,0);
   glScaled(s,s,s);
   glDrawElements(GL_TRIANGLES,N,GL_UNSIGNED_BYTE,index);
   // switch(pile) {
   //    case 5:
   //       glRotated(225,0,1,0);
   //       glTranslated(1,1,1); 
   //       glDrawElements(GL_TRIANGLES,N,GL_UNSIGNED_BYTE,index);
   //    case 4:
   //       glRotated(180,0,1,0);
   //       glTranslated(x,y,z);
   //       glDrawElements(GL_TRIANGLES,N,GL_UNSIGNED_BYTE,index);
   //    case 3:
   //       glRotated(135,0,1,0);
   //       glTranslated(x,y,z);
   //       glDrawElements(GL_TRIANGLES,N,GL_UNSIGNED_BYTE,index);
   //    case 2:
   //       glRotated(90,0,1,0);
   //       glTranslated(x,y,z);
   //       glDrawElements(GL_TRIANGLES,N,GL_UNSIGNED_BYTE,index);
   //    case 1:
   //       glRotated(45,0,1,0);
   //       glTranslated(x,y,z);
   //       glDrawElements(GL_TRIANGLES,N,GL_UNSIGNED_BYTE,index);
   // }
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
   //  Disable vertex array
   glDisableClientState(GL_VERTEX_ARRAY);
   //  Disable color array
   glDisableClientState(GL_COLOR_ARRAY);
   // Disable texture array
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   // Disable normal array
   glDisableClientState(GL_NORMAL_ARRAY);
}

static void drawgrass(double x,float y,double z,float s,float shade)
{
   s = s/100;
   double dres = (double)res;
   double offset = (double)(width_bound*res)/2;
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,.001,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   // glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   double newpx = px+offset/dres;
   double newpz = pz+offset/dres;
   double angle = 180*atan2(newpx - x,newpz - z)/PI;
   glBindTexture(GL_TEXTURE_2D,texture[4]);
   double speed = 1;
   double factor = moment*fmod(x * z, 2)*speed;
   double sway = (Sin(factor) - Sin(factor/2) + Sin(factor/4) - Sin(factor/8))/4;
   double bottom = .2 * sway;
   double middle = .66 * sway;
   double top = 1.2 * sway;
   double bottomh = sqrt(.35-pow(bottom,2));
   double middleh = sqrt(1.2-pow(middle,2));
   double toph = sqrt(2-pow(top,2));
   //  Enable blending
   // glEnable(GL_BLEND);  
   glPushMatrix();
   glColor4f(.6+shade,.6+shade,.6+shade,1);

   glTranslated(x,y,z);
   glRotated(angle,0,1,0);
   glScaled(s*1,s*4,s*1);

   glBegin(GL_TRIANGLE_STRIP);
   glNormal3f(-Sin(angle),0,Cos(angle));
   glTexCoord2f(0,0); glVertex3f(.25,0,0); 
   glTexCoord2f(0,.5); glVertex3f(0,0,0);

   glTexCoord2f(.3,0); glVertex3f(.33+bottom,bottomh,0);
   glTexCoord2f(.3,.5); glVertex3f(0+bottom,bottomh,0);

   glTexCoord2f(.6,0); glVertex3f(.25+middle,middleh,0);
   glTexCoord2f(.6,.5); glVertex3f(0+middle,middleh,0);

   glTexCoord2f(1,.5); glVertex3f(0+top,toph,0);
   
   glTexCoord2f(.6,.5); glVertex3f(0+middle,middleh,0);
   glTexCoord2f(.6,1); glVertex3f(-.25+middle,middleh,0);

   glTexCoord2f(.3,.5); glVertex3f(0+bottom,bottomh,0);
   glTexCoord2f(.3,1); glVertex3f(-.33+bottom,bottomh,0);

   glTexCoord2f(0,.5); glVertex3f(0,0,0);
   glTexCoord2f(0,1); glVertex3f(-.25,0,0);

   glEnd();
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);
}

static void drawtrail(float color[]) {
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,.01,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   // glPushMatrix();
   glTranslated(0,.005,0);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   // glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[6]);
   glBegin(GL_TRIANGLES);
   int panels = 1;
   double step = 1.0/panels;
   // float color2[3] = {0.24, 0.19, 0.18};
   // float color3[3] = {color[0],color[1],color[2]};
   double dres = (double)res;
   float y = 0;
   double newi = 0;
   double newj = 0;
   int startz = width_bound*res/2;
   int x = 0;
   int z = startz + (startz-1)%2;
   // color[0] = 0.24;
   // color[1] = 0.19;
   // color[2] = 0.18;
   double normals[3] = {0,0,0};
   while(xpath[z] > 0) {
      x = xpath[z];
      if(world_map[x][z][1] == 5) {
         for(int block = -1; block <= 1; block+=2) {
               y = world_map[x][z][0]+.1;
               newi = (double)x/dres;
               newj = (double)z/dres;
               surfacenormal(normals,x,z);
               glNormal3f(normals[0],normals[1],normals[2]);
               // glNormal3f(newi,fabs(y),newj);
               glColor3f(color[0],color[1],color[2]);
               glTexCoord2f((double)(z % panels)*step,(double)(x % panels)*step); 
               glVertex3d(newi,y/dres,newj);

               z = z-block;
               y = world_map[x][z][0];
               newi = (double)x/dres;
               newj = (double)z/dres;
               surfacenormal(normals,x,z);
               glNormal3f(normals[0],normals[1],normals[2]);
               glColor3f(color[0],color[1],color[2]);
               glTexCoord2f((double)(z % panels)*step+step,(double)(x % panels)*step+step); 
               glVertex3d(newi,(y)/dres,newj);

               x = x-block;
               z = z+block;
               y = world_map[x][z][0];
               newi = (double)x/dres;
               newj = (double)z/dres;
               surfacenormal(normals,x,z);
               glNormal3f(normals[0],normals[1],normals[2]);
               glColor3f(color[0],color[1],color[2]);
               glTexCoord2f((double)(z % panels)*step,(double)(x % panels)*step + step); 
               glVertex3d(newi,y/dres,newj);

               x = x+block;

               // Offset blocks
               y = world_map[x][z][0]+.1;
               newi = (double)x/dres;
               newj = (double)z/dres;
               surfacenormal(normals,x,z);
               glNormal3f(normals[0],normals[1],normals[2]);
               glColor3f(color[0],color[1],color[2]);
               glTexCoord2f((double)(z % panels)*step+step,(double)(x % panels)*step+step); 
               glVertex3d(newi,y/dres,newj);

               z = z-block;
               y = world_map[x][z][0];
               newi = (double)x/dres;
               newj = (double)z/dres;
               surfacenormal(normals,x,z);
               glNormal3f(normals[0],normals[1],normals[2]);
               glColor3f(color[0],color[1],color[2]);
               glTexCoord2f((double)(z % panels)*step+step,(double)(x % panels)*step); 
               glVertex3d(newi,y/dres,newj);
               
               x = x+block;
               z = z+block;
               y = world_map[x][z][0];
               newi = (double)x/dres;
               newj = (double)z/dres;
               surfacenormal(normals,x,z);
               glNormal3f(normals[0],normals[1],normals[2]);
               glColor3f(color[0],color[1],color[2]);
               glTexCoord2f((double)(z % panels)*step,(double)(x % panels)*step); 
               glVertex3d(newi,(y)/dres,newj);

               x = x-block;
         }
      }
      z++;
   }
   glEnd();
   // glPopMatrix();
   glDisable(GL_TEXTURE_2D);
}

static void plank(float x, float y, float z,float pitch, float yaw, float roll, float s) {
   double dres = (double)res;
   float depth = .8;
   float width = depth * 6.66666;
   float height = depth*.533333;
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float black[] = {0,0,0,1};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);

   glPushMatrix();

   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(0.45, 0.35, 0.23);
   // glColor3f(1,1,1);

   //  Offset, scale and rotate
   glTranslated(x/dres,y/dres,z/dres);
   glRotated(yaw,0,1,0);
   glRotated(pitch,1,0,0);
   glRotated(roll,0,0,1);
   glScaled(s/dres,s/dres,s/dres);

   width = width/2;
   height = height/2;
   depth = depth/2;
   double length = (height+depth)*2;
   //  Cube
   glBindTexture(GL_TEXTURE_2D,texture[5]);
   glBegin(GL_QUADS);
   //  Front
   glNormal3f( 0, 0, 1);
   glTexCoord2f(0,0); glVertex3f(-width,-height, depth);
   glTexCoord2f(0,height/length); glVertex3f(-width,+height, depth);
   glTexCoord2f(1,height/length);glVertex3f(+width,+height, depth);
   glTexCoord2f(1,0); glVertex3f(+width,-height, depth);
   //  Back
   glNormal3f( 0, 0,-1);
   glTexCoord2f(0,(height+depth)/length); glVertex3f(-width,+height,-depth);
   glTexCoord2f(0,(2*height+depth)/length); glVertex3f(-width,-height,-depth);
   glTexCoord2f(1,(2*height+depth)/length); glVertex3f(+width,-height,-depth);
   glTexCoord2f(1,(height+depth)/length); glVertex3f(+width,+height,-depth);
   //  Top
   glNormal3f( 0,+1, 0);
   glTexCoord2f(0,height/length); glVertex3f(-width,+height,+depth);
   glTexCoord2f(0,(height+depth)/length); glVertex3f(-width,+height,-depth);
   glTexCoord2f(1,(height+depth)/length); glVertex3f(+width,+height,-depth);
   glTexCoord2f(1,height/length); glVertex3f(+width,+height,+depth);
   //  Bottom
   glNormal3f( 0,-1, 0);
   glTexCoord2f(0,(2*height+depth)/length); glVertex3f(-width,-height,-depth);
   glTexCoord2f(0,2*(height+depth)/length); glVertex3f(-width,-height,+depth);
   glTexCoord2f(1,2*(height+depth)/length); glVertex3f(+width,-height,+depth);
   glTexCoord2f(1,(2*height+depth)/length); glVertex3f(+width,-height,-depth);
   glColor3f(0.35, 0.25, 0.13);
   //  Right
   glNormal3f(+1, 0, 0);
   glTexCoord2f(0,0); glVertex3f(+width,-height,+depth);
   glTexCoord2f(0,1); glVertex3f(+width,+height,+depth);
   glTexCoord2f(1,1); glVertex3f(+width,+height,-depth);
   glTexCoord2f(1,0); glVertex3f(+width,-height,-depth);
   //  Left
   glNormal3f(-1, 0, 0);
   glTexCoord2f(0,0); glVertex3f(-width,+height,-depth);
   glTexCoord2f(1,0); glVertex3f(-width,+height,+depth);
   glTexCoord2f(1,1); glVertex3f(-width,-height,+depth);
   glTexCoord2f(0,1); glVertex3f(-width,-height,-depth);
   //  End
   glEnd();
   //  Undo transofrmations
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
}

static void drawbridges() {
   double dres = (double) res;
   int alpha = 0;
   float elevation = 0;
   double pitch = 0;

   int b = 0;
   while(bridges[b].true) {
      int heading = bridges[b].heading;
      glPushMatrix();
      glTranslated(bridges[b].startx,bridges[b].starth,bridges[b].startz);
      glRotated(heading,0,1,0);

      elevation = (bridges[b].endh-bridges[b].starth)/4;
      pitch = -sin(elevation)*180.0;
      glTranslated(0,0,-.5/dres);
      float z = bridges[b].startz;
      while(z < bridges[b].endz) {
         plank(0,0,0,pitch,0,0,.5);
         glTranslated(0,elevation/4,.5/dres);
         glRotated(alpha,0,1,0);
         z += (.5/dres)*cos(heading);

      }
      glPopMatrix();
      b++;
   }
}

static void drawriver() {
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float black[] = {0,0,0,1};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   // glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[8]);
   //  Enable blending
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(0,0,0);
   glScaled(1/(double)res,1/(double)res,1/(double)res);
   glBegin(GL_QUADS);
   glColor4f(0.09, 0.23, 0.35,1);
   // glVertex3f(riverpath[0],0,0);
   double flow = -6*(moment-180.0)/360;
   for(int i = 1; i < MAX_RIVER-10; i++) {
      if(riverpath[i][1] > length_bound*res-2) {
         break;
      }
      glNormal3f(0, 1, 0);
      glTexCoord2f(-.5,-.5+flow); glVertex3f(riverpath[i][0]+2,world_map[(int)riverpath[i][0]+1][(int)riverpath[i][1]][0],(int)riverpath[i][1]);
      glTexCoord2f(-.5,.5+flow); glVertex3f(riverpath[i+1][0]+2,world_map[(int)riverpath[i+1][0]+1][(int)riverpath[i+1][1]][0],(int)riverpath[i+1][1]);
      glTexCoord2f(.5,.5+flow); glVertex3f(riverpath[i+1][0]-2,world_map[(int)riverpath[i+1][0]-1][(int)riverpath[i+1][1]][0],(int)riverpath[i+1][1]);
      glTexCoord2f(.5,0+flow); glVertex3f(riverpath[i][0]-2,world_map[(int)riverpath[i][0]-1][(int)riverpath[i][1]][0],(int)riverpath[i][1]);
      
   }
   glEnd();
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);

}

int trailbuild(int startx, int startz) {
   double dres = (double)res;
   int x = startx + (startx-1)%2;
   int z = startz + (startz-1)%2;
   for(int i = 0; i < z; i++) {
      xpath[i] = OOB;
   }
   xpath[z] = x;
   int nextx = pow(-1,(rand()%2)+1);
   int strikes = 0;
   int steps = 5;
   // Build the trail on the world map
   while(z < length_bound*res-res && z >= startz) {
      if(z % steps == 0) {
         nextx = pow(-1,(rand()%2)+1);
      }
      if(x+nextx < (double)width_bound*dres-2 && x+nextx > 2 && world_map[x+nextx][z+1][1] >= 0) {
         x += nextx;
         z += 1;
         xpath[z] = x;
         world_map[x][z][1] = TRAIL;
         world_map[x][z][0] -= .05;
         strikes = 0;
      }
      else if(strikes <= 2) {
         strikes++;
         nextx = -nextx;
      }
      else if(strikes > 2) {
         world_map[x][z][1] = OOB;
         z = z - 1;
         x = xpath[z];
         strikes = 0;
      }
   }
   if(z <= startz) {
      return 0;
   }
   

   // Place bridges with value 6
   z = startz+5;
   int num_bridges = 0;
   while(z < length_bound*res-res-10 && num_bridges < MAX_BRIDGES) {
      // / Failed attempt to make the bridges connect any two high points. Swapped to bridges only over water
      // if(world_map[(int)xpath[z]][z][0] > world_map[(int)xpath[z-1]][z-1][0] && world_map[(int)xpath[z]][z][0] > world_map[(int)xpath[z+1]][z+1][0]) {
      //    if(!bridge) {
      //       startz = z+1;
      //       start_height = world_map[(int)xpath[z]][z][0];
      //       heading = (xpath[startz]-xpath[z]) * 45;
      //       printf("%d\n",heading);
      //       bridge = !bridge;
      //       world_map[(int)xpath[z]][z][1] = ROCK;
      //    }
      //    else if(z >= startz + 3) {
      //       struct bridge newBridge;
      //       newBridge.startz = startz;
      //       newBridge.endz = z-1;
      //       newBridge.heading = heading;
      //    }
      // }
      int b_length = 2;
      if(world_map[(int)xpath[z]][z][1] == RIVER || world_map[(int)xpath[z]][z+1][1] == RIVER || world_map[(int)xpath[z]+1][z][1] == RIVER || world_map[(int)xpath[z]][z-1][1] == RIVER || world_map[(int)xpath[z]-1][z][1] == RIVER || world_map[(int)xpath[z]+1][z+1][1] == RIVER || world_map[(int)xpath[z]+1][z-1][1] == RIVER || world_map[(int)xpath[z]-1][z-1][1] == RIVER || world_map[(int)xpath[z]-1][z+1][1] == RIVER) {
         struct bridge newBridge;
         newBridge.true = 1;
         newBridge.startz = (z - b_length)/dres;
         newBridge.startx = (xpath[z-b_length])/dres;
         newBridge.endz = (z + b_length)/dres;
         newBridge.endx = xpath[z+b_length]/dres;
         newBridge.starth = (world_map[(int)xpath[z-b_length-1]][z - b_length-1][0]+world_map[(int)xpath[z-b_length]][z - b_length][0])/2/dres;
         newBridge.endh =  (world_map[(int)xpath[z+b_length]][z + b_length][0] + world_map[(int)xpath[z+b_length+1]][z + b_length+1][0])/2/dres;
         newBridge.heading = (xpath[z]-xpath[z-1]) * 45;
         bridges[num_bridges] = newBridge;
         for(int bz = z-b_length; bz <= z+b_length; bz++) {
            world_map[(int)xpath[bz]][bz][1] = BRIDGE;
         }
         num_bridges++;
         z += 3;
      }
      z++;
   }
   for(int i = num_bridges; i < MAX_BRIDGES; i++) {
      struct bridge noBridge;
      noBridge.true = 0;
   }
   return 1;
}

static void reset() {
   px = 0;
   pz = 0;
   for(int j = 0; j < length_bound*res; j++) {
      for(int i = 0; i < width_bound*res; i++) {
         // printf("%.0f ",world_map[i][j][1]);
         world_map[i][j][0] = 0;
         world_map[i][j][1] = 0;
      }
      xpath[j] = -1;
      // printf("\n");
   }
   for(int i = 0; i < MAX_RIVER; i++) {
      riverpath[i][0] = -1;
      riverpath[i][1] = -1;
   }
   // world_map[width_bound*res/2][width_bound*res/2][1] = -1;
}

static void init() {
   double dres = (double)res;
   reset();
   int ele = 20; //average elevation change
   double step = 0; //step elevation accumulator
   int steepness = 3; //steepness per step
   int smoothness = 3; //overal smoothness of the surface

   for(int j = 0; j < length_bound*res; j++) {
      world_map[0][j][0] = step;
      for(int i = 0; i < width_bound*res; i++) {
         // Elevation Points
         world_map[i][j][0] = (double)(rand()%ele) + fabs(((double)width_bound*dres/2.0) - (double)i)/3 - step;
      }
      if(j % width_bound*res == 0) {
         step = step + (double)((rand() % steepness));
      }
   }

   // SMOOTHING ELEVATION DATA
   for(int k = 0;k<smoothness;k++) {
      for(int i = 1; i < width_bound*res-1; i++) {
         for(int j = 1; j < length_bound*res-1; j++) {
            world_map[i][j][0] = (world_map[i-1][j-1][0]+world_map[i-1][j+1][0]+world_map[i+1][j-1][0]+world_map[i+1][j+1][0]+world_map[i+1][j][0]+world_map[i][j+1][0]+world_map[i-1][j][0]+world_map[i][j-1][0])/8.0;
         }
      }
      for(int i = 0; i < width_bound*res; i++) {
            world_map[i][0][0] = (world_map[i][1][0]);
            world_map[i][length_bound*res-1][0] = (world_map[i][length_bound*res-2][0]);
      }
      for(int j = 0; j < length_bound*res; j++) {
            world_map[0][j][0] = (world_map[1][j][0]);
            world_map[width_bound*res-1][j][0] = (world_map[width_bound*res-2][j][0]);
      }
   }

   // populate the mountain with trees and rocks 
   int tree_freq = 5; //tree frequency
   int rock_freq = 10; //rock frequency
   int grass_freq = 4;
   for(int i = 1; i < width_bound*res-1; i++) {
      for(int j = 1; j < length_bound*res-1; j++) {
         
         if(world_map[i][j][1] == EMPTY && (rand() % 100)+1 < 1+tree_freq*abs(i-width_bound*res/2)/dres) {
            struct tree newTree;
            newTree.x = i/dres; //x
            newTree.y = world_map[i][j][0]/dres; //y
            newTree.z = j/dres; //z
            newTree.height = (double)(rand() % (int)(50))/100.0 + 2; //height
            // newTree.height = .5+ 0*abs(i-width_bound*res/2)/dres; //height
            // newTree.width = .4 + .5*(double)abs(i-width_bound*res/2)/dres; //width
            newTree.width = (double)(rand() % (int)(50))/100.0 + .8; //width
            newTree.scale = (double)(rand() % 20)/100 + .9; //
            newTree.shade = (double)(rand()%20)/100.0 + .2;
            trees[i][j] = newTree;
            world_map[i-1][j][1] = OOB;
            world_map[i+1][j][1] = OOB;
            world_map[i][j-1][1] = OOB;
            world_map[i][j+1][1] = OOB;
            world_map[i-1][j-1][1] = OOB;
            world_map[i+1][j+1][1] = OOB;
            world_map[i+1][j-1][1] = OOB;
            world_map[i-1][j+1][1] = OOB;
            world_map[i][j][1] = TREE;
         }
         if(world_map[i][j][1] == EMPTY && (rand() % 100)+1 < rock_freq) {
            struct rockpatch newPatch;
            for(int k=0;k<MAX_ROCK_PATCH;k++) {
               for(int l=0;l<MAX_ROCK_PATCH;l++) {
                  struct rock newRock;
                  if((rand()%100)+1 < 80*1/abs(k-MAX_ROCK_PATCH/2)) {
                     newRock.x = ((double)i+1*(double)k/(double)MAX_ROCK_PATCH)/dres; //x
                     newRock.z = ((double)j+1*(double)l/(double)MAX_ROCK_PATCH)/dres; //z
                     newRock.y = findY(newRock.x,newRock.z) + (double)width_bound/2.0 - .01; //y
                     newRock.true = 1;
                     newRock.th = (double)(rand() % 360); //angle   
                     newRock.s = (double)(rand()%50)/100 + .2;
                     newRock.stretch = (double)(rand() % 1500)/1000 - 1.5;
                  }
                  else {
                     newRock.true = 0;
                  }
                  newPatch.patch[k][l] = newRock;
               }
            }
            rocks[i][j] = newPatch;
            world_map[i][j][1] = ROCK;
         }
      }
   }
   for(int i = 6; i < width_bound*res-6; i++) {
      for(int j = 6; j < length_bound*res-6; j++) {
         struct grasspatch newPatch;
         if((rand() % 100)+1 < grass_freq) {
            double var_freq = rand() % 20 + 8;
            for(int k=0;k<MAX_GRASS_PATCH;k++) {
               for(int l=0;l<MAX_GRASS_PATCH;l++) {
                  struct grass newGrass;
                  double center = fabs((double)k-(double)MAX_GRASS_PATCH/2.0)+fabs((double)l-(double)MAX_GRASS_PATCH/2.0);
                  if((rand()%100) > (var_freq*(center))) {
                     newGrass.x = ((double)i+4*(double)k/(double)MAX_GRASS_PATCH)/dres; //x
                     newGrass.z = ((double)j+4*(double)l/(double)MAX_GRASS_PATCH)/dres; //z
                     newGrass.y = findY(newGrass.x,newGrass.z) + (double)width_bound/2.0 - .005; //y
                     newGrass.true = 1;
                     newGrass.s = (double)(rand()%110)/100 + .1;
                     newGrass.shade = (double)(rand()%20)/100;
                  }
                  else {
                     newGrass.true = 0;
                  }
                  newPatch.patch[k][l] = newGrass;
               }
            }
            newPatch.true = 1;
         }
         else {
            newPatch.true = 0;
         }
         grass[i][j] = newPatch;
      }
   }

   // Initialize river position
   int minx = 0;
   int minz = 0;
   // Find intitial minimum elevation at x
   for(int x = 2; x < width_bound*res-2; x++) {
      if(world_map[x][minz][0] < world_map[minx][minz][0]) {
         minx = x;
      }
   }
   riverpath[0][0] = minx;
   riverpath[0][1] = minz;
   int i = 0;
   double lowest = INFINITY;
   while(i < MAX_RIVER && minz < length_bound*res-1) {
      // for(int z = 0; z <= 1; z++) {
         for(int x = -1; x <= 1; x+=1) {
            if(world_map[(int)riverpath[i][0]+x][(int)riverpath[i][1]+1][0] < lowest && world_map[(int)riverpath[i][0]+x][(int)riverpath[i][1]+1][1] != RIVER) {
               minx = riverpath[i][0]+x;
               minz = riverpath[i][1] + 1;
               lowest = world_map[minx][minz][0];
            }
         // }
      }
      i++;
      riverpath[i][0] = minx;
      riverpath[i][1] = minz;
      world_map[minx][minz][1] = RIVER;
      world_map[minx][minz][0] -= .5;
      world_map[minx+1][minz][0] -= .5;
      world_map[minx-1][minz][0] -= .5;
      lowest = INFINITY;
   }
   // Initialize trail position
   while(!trailbuild(width_bound*res/2,width_bound*res/2)){
      printf("Trailbuild Failed");
      init();
   };
}

/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */
void display()
{
   //  Erase the window and the depth buffer
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   //  Enable Z-buffering in OpenGL
   glEnable(GL_DEPTH_TEST);
   //  Undo previous transformations
   glLoadIdentity();
   
   // Calculations for camera positions both first person and perspective views
   Cx = -dim*Sin(th)*Cos(ph)*dist + px;
   Cy =  dim   *Sin(ph)*dist + py;
   Cz =  dim*Cos(th)*Cos(ph)*dist + pz;
   
   if(mode == 0) {
      // First Person
      double offset = ((double)width_bound/2.0);
      py = findY(px+offset,pz+offset) + .2;
      gluLookAt(px,py,pz,Cx,Cy,Cz, 0,Cos(ph),0);
   }
   else { 
      // World View
      gluLookAt((Cx-px)*dist,(Cy-py)*dist,(Cz-pz)*dist, 0,0,0 , 0,Cos(ph),0);
   }

   // // Draw Sky
   glPushMatrix();
   // Follows the player if in first person mode
   if(mode == 0) {
      glTranslated(px,py,pz);
   }
   else {
      glTranslated(0,0,0);
   }
   skycube(0,0,0,length_bound,0,white);
   glPopMatrix();

   // Smooth shading
   glShadeModel(GL_SMOOTH);
   glDisable(GL_LIGHTING);
   //  Light switch
   if(light) {
      //  Translate intensity to color vectors
      float Ambient[]   = {0.01*ambient ,0.01*ambient ,0.01*ambient ,1.0};
      float Diffuse[]   = {0.01*diffuse ,0.01*diffuse ,0.01*diffuse ,1.0};
      float Specular[]  = {0.01*specular,0.01*specular,0.01*specular,1.0};
      //  Light position
      float Position[]  = {dim*Cos(zh)*(double)distance,ylight+dim*(double)distance,dim*Sin(zh)*(double)distance,1.0};
      //  Draw light position as ball (still no lighting here)
      glColor3f(1,1,1);
      ball(Position[0],Position[1],Position[2] , 0.05);
      //  OpenGL should normalize normal vectors
      glEnable(GL_NORMALIZE);
      //  Enable lighting
      glEnable(GL_LIGHTING);
      //  Location of viewer for specular calculations
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,local);
      //  glColor sets ambient and diffuse color materials
      glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
      //  Enable light 0
      glEnable(GL_LIGHT0);
      //  Set ambient, diffuse, specular components and position of light 0
      glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
      glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
      glLightfv(GL_LIGHT0,GL_POSITION,Position);
   }
   else {
      glDisable(GL_LIGHTING);
   }

   glUseProgram(0);
   // Draw scenes
   switch(world) {
      case 0: // Mountain World
      {
         glPushMatrix();
         glTranslated(-(double)width_bound/2.0,-(double)width_bound/2.0,-(double)width_bound/2.0);
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(2,2);
         // Draw base surface
         surface(white);
         glPolygonOffset(1,1);
         // Draw the trail
         drawtrail(dirt);
         glDisable(GL_POLYGON_OFFSET_FILL);
         // Draw the bridges
         drawbridges();
         // Draw the river
         drawriver();
         //  Draw world objects
         for(int i = 0; i<width_bound*res; i++) {
            for(int j = 0; j<length_bound*res; j++) {
               if(world_map[i][j][1] == TREE) {
                  drawtree(trees[i][j].x,trees[i][j].y,trees[i][j].z,trees[i][j].height,trees[i][j].width,trees[i][j].scale,trees[i][j].shade);
               }
               if(world_map[i][j][1] == ROCK) {
                  for(int k=0;k<MAX_ROCK_PATCH;k++) {
                     for(int r=0;r<MAX_ROCK_PATCH;r++) {
                        if(rocks[i][j].patch[k][r].true == 1) {
                           drawrock(rocks[i][j].patch[k][r].x,rocks[i][j].patch[k][r].y,rocks[i][j].patch[k][r].z,rocks[i][j].patch[k][r].th,rocks[i][j].patch[k][r].s,rocks[i][j].patch[k][r].stretch);
                        }
                     }
                  }
               }
               if(world_map[i][j][1] != RIVER && world_map[i][j][1] != TRAIL && grass[i][j].true) {
                  for(int k=0;k<MAX_GRASS_PATCH;k++) {
                     for(int r=0;r<MAX_GRASS_PATCH;r++) {
                        if(grass[i][j].patch[k][r].true == 1) {
                           drawgrass(grass[i][j].patch[k][r].x,grass[i][j].patch[k][r].y,grass[i][j].patch[k][r].z,grass[i][j].patch[k][r].s,grass[i][j].patch[k][r].shade);
                        }
                     }
                  }
               }
            }
         }
         glPopMatrix();
         break;
      }
      case 1: // Test
      {
         mode = 1;
         switch(item) {
            case 0: {
               drawrock(0,0,0,0,5,1);
               break;
            }
            case 1: {
               drawtree(0,0,0,1,1,3,.5);
               break;
            }
            case 2: {
               drawgrass(0,0,0,20,0);
               break;
            }
            case 3: {
               glPushMatrix();
               glTranslated(-(double)width_bound/2.0,-(double)width_bound/2.0,-(double)width_bound/2.0);
               surface(white);
               drawriver();
               drawtrail(dirt);
               drawbridges();
               glPopMatrix();
            }
         }
         break;
      }
   }

   glUseProgram(0);
   glDisable(GL_LIGHTING);
   glColor3f(1,1,1);
   if(axes) {
      //  Draw axes
      const double len=dim*1.5;  //  Lllength of axes
      glBegin(GL_LINES);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(len,0.0,0.0);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(0.0,len,0.0);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(0.0,0.0,len);
      glEnd();
      //  Label axes
      glRasterPos3d(len,0.0,0.0);
      Print("X");
      glRasterPos3d(0.0,len,0.0);
      Print("Y");
      glRasterPos3d(0.0,0.0,len);
      Print("Z");
   }

   // Print light switch
   glWindowPos2i(5,135);
   char * onoff[2] = {"OFF","ON"};
   Print("Light = %s",onoff[light]);

   // Print the current mode on screen
   glWindowPos2i(5,110);
   char * modes[3] = {"FIRST PERSON","PERSPECTIVE","ORTHOGRAPHIC"};
   Print("Mode = %s",modes[mode]);
      
   // Print the current scene on screen
   glWindowPos2i(5,85);
   char * worlds[2] = {"MOUNTAIN","TEST"};
   Print("Scene = %s",worlds[world]);

   if(world) {
      glWindowPos2i(5,60);
      // Print out the current item on screen
      char* items[5] = {"ROCK","TREE","PLANT","SURFACE"};
      Print("Item = %s",items[item]);
   }

   //  Display parameters
   glWindowPos2i(5,10);
   // View Angle, Dimension, and Field of View
   Print("Angle th: %d, ph: %d  zh=%d FOV=%d Dist=%.1f",th,ph,zh,fov,dist);

   // // Player and camera position data a for debugging
   // glWindowPos2i(5,30);
   // Print("Player X: %f, Player Y: %f, Player Z: %f",px,py,pz);
   // glWindowPos2i(5,5);
   // Print("Camera X: %f, Camera Y: %f, Camera Z: %f",Cx,Cy,Cz);
   //  Render the scene and make it visible

   ErrCheck("display");
   glFlush();
   glutSwapBuffers();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void idle()
{
   if(pause) {
         //  Elapsed time in seconds
      double t = (glutGet(GLUT_ELAPSED_TIME) - pausetime)/1000.0;
      // zh = (360 - (fmod(90*t,220)))-140;
      zh = fmod(90*t*.5,360);
      moment = 90*t;
      //  Tell GLUT it is necessary to redisplay the scene
      glutPostRedisplay();
   }
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key,int x,int y)
{
   //  Right arrow key - increase angle by 5 degrees
   if (key == GLUT_KEY_RIGHT)
      th += 5;
   //  Left arrow key - decrease angle by 5 degrees
   if (key == GLUT_KEY_LEFT)
      th -= 5;
   //  Up arrow key - increase elevation by 5 degrees
   if (key == GLUT_KEY_UP && ph < 85)
      ph += 5;
   //  Down arrow key - decrease elevation by 5 degrees
   if (key == GLUT_KEY_DOWN && ph > -85)
      ph -= 5;
   //  Keep angles to +/-360 degrees
   th %= 360;
   ph %= 360;
   //  Update projection
   Project(fov,asp,dim,mode);
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a key is pressed
 */
void key(unsigned char ch,int x,int y)
{
   double a[3] = {Cx-px,0,Cz-pz}; // Vector from camera to focus (camera direction)
   double amag = sqrt(pow(a[0],2)+pow(a[1],2) + pow(a[2],2)); //magnitude of camera direction
   double b[3] = {0,1,0}; // player up vector
   double c[3] = cross(a[0],b[0],a[1],b[1],a[2],b[2]); // cross product of camera direction and up. Gives strafe vector
   double cmag = sqrt(pow(c[0],2)+pow(c[1],2)+pow(c[2],2)); // magnitude of strafe vector
   double d = 0.01; //desired incremental movement distance value
   double addx = 0;
   double addz = 0;
   //  Exit on ESC
   if (ch == 27)
      exit(0);
   //  Reset view angle and player position
   if (ch == '0') {
      th = ph = px = pz = 0;
   }
   // Re-initialize mountain world surface and item instances
   if(ch == 'r' || ch == 'R') {
      init();
   }
   // Change Viewing mode (First Person, Perspective, Orthographic)
   if (ch == 'm' || ch == 'M') {
      mode = (mode + 1) % 3;
      if(mode == 0 || mode == 1) {
         th = (th-180)%360;
         ph = -ph;
      }
   }
   // Change World (Mountain, Test)
   if (ch == 'n' || ch == 'N') {
      world = (world+1) % 2;
   }
   // Change Item in Test World
   if (ch == 'b' || ch == 'B') {
      item = (item +1) % 4;
   }
   // Pause time
   if (ch == 'p' || ch == 'P') {
         if(!pause) {
            pausetime = glutGet(GLUT_ELAPSED_TIME) - pausetime;
         }
         else {
            pausetime = glutGet(GLUT_ELAPSED_TIME) - pausetime;
         }
         pause = !pause;
   }
   // Turn Light On or Off
   if (ch == 'l' || ch == 'L') {
         light = !light;
   }
   //  Toggle axes
   if (ch == 't' || ch == 'T')
      axes = ~axes;
   // //  Change field of view angle
   // if (ch == '-' && fov>1) {
   //    fov--;
   // }
   // if (ch == '=' && fov<179) {
   //    fov++;
   // }
   //  d key. move right by .25 units
   if ((ch == 'd' || ch == 'D')) {
      if(mode == 0) {
         addx += (c[0]/cmag) * d;
         addz += (c[2]/cmag) * d;
      }
      else {
         addx -= 2*(c[0]/cmag) * d;
         addz -= 2*(c[2]/cmag) * d;
      }
   }
   //  a key. move left by .25 units
   if ((ch == 'a' || ch == 'A')) {
      if(mode == 0) {
         addx -= (c[0]/cmag) * d;
         addz -= (c[2]/cmag) * d;
      }
      else {
         addx += 2*(c[0]/cmag) * d;
         addz += 2*(c[2]/cmag) * d;
      }
   }
   //  w key. move forward by .25 units
   if ((ch == 'w' || ch == 'W')) {
      if(mode == 0) {
         addx += a[0]*d/amag;
         addz += a[2]*d/amag;
      }
      else {
         addx -= 2*a[0]*d/amag;
         addz -= 2*a[2]*d/amag;
      }
   }
   //  s key. move back by .25 unites
   if ((ch == 's' || ch == 'S')) {
      if(mode == 0) {
         addx -= a[0]*d/amag;
         addz -= a[2]*d/amag;
      }
      else {
         addx += 2*a[0]*d/amag;
         addz += 2*a[2]*d/amag;
      }
   }
   //  period - increase dim
   if (ch == '.' && dim<10)
      dist += 0.1;
   //  comma - decrease dim
   if (ch == ',' && dim>0)
      dist -= 0.1;
   
   // Check to make sure our movement is within the world's bounds and free of obstacles before adding it
   if(fabs(px+addx) < ((double)width_bound/2)-(double)width_bound/(double)res) {
      px += addx;
   }
   if(fabs(pz+addz) < ((double)length_bound)-(double)length_bound/(double)res - (double)width_bound/2 && pz+addz > -(double)width_bound/2 + (double)width_bound/(double)res) {
      pz += addz;
   }
   //  Reproject
   Project(fov,asp,dim,mode);
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void reshape(int w,int h)
{
   //  Ratio of the width to the height of the window
   asp = (h>0) ? (double)w/h : 1;
   //  Set the viewport to the entire window
   glViewport(0,0, RES*w,RES*h);
   win_width = w;
   win_height = h;
   //  Set projection
   Project(fov,asp,dim,mode);
}

/*
 *  Read text file
 */
char* ReadText(char *file)
{
   char* buffer;
   //  Open file
   FILE* f = fopen(file,"rt");
   if (!f) Fatal("Cannot open text file %s\n",file);
   //  Seek to end to determine size, then rewind
   fseek(f,0,SEEK_END);
   int n = ftell(f);
   rewind(f);
   //  Allocate memory for the whole file
   buffer = (char*)malloc(n+1);
   if (!buffer) Fatal("Cannot allocate %d bytes for text file %s\n",n+1,file);
   //  Snarf the file
   if (fread(buffer,n,1,f)!=1) Fatal("Cannot read %d bytes for text file %s\n",n,file);
   buffer[n] = 0;
   //  Close and return
   fclose(f);
   return buffer;
}

/*
 *  Print Shader Log
 */
void PrintShaderLog(int obj,char* file)
{
   int len=0;
   glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for shader log\n",len);
      glGetShaderInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s:\n%s\n",file,buffer);
      free(buffer);
   }
   glGetShaderiv(obj,GL_COMPILE_STATUS,&len);
   if (!len) Fatal("Error compiling %s\n",file);
}

/*
 *  Print Program Log
 */
void PrintProgramLog(int obj)
{
   int len=0;
   glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&len);
   if (len>1)
   {
      int n=0;
      char* buffer = (char *)malloc(len);
      if (!buffer) Fatal("Cannot allocate %d bytes of text for program log\n",len);
      glGetProgramInfoLog(obj,len,&n,buffer);
      fprintf(stderr,"%s\n",buffer);
   }
   glGetProgramiv(obj,GL_LINK_STATUS,&len);
   if (!len) Fatal("Error linking program\n");
}

/*
 *  Create Shader
 */
int CreateShader(GLenum type,char* file)
{
   //  Create the shader
   int shader = glCreateShader(type);
   //  Load source code from file
   char* source = ReadText(file);
   glShaderSource(shader,1,(const char**)&source,NULL);
   free(source);
   //  Compile the shader
   fprintf(stderr,"Compile %s\n",file);
   glCompileShader(shader);
   //  Check for errors
   PrintShaderLog(shader,file);
   //  Return name
   return shader;
}

/*
 *  Create Shader Program
 */
int CreateShaderProg(char* VertFile,char* FragFile)
{
   //  Create program
   int prog = glCreateProgram();
   //  Create and compile vertex shader
   int vert = CreateShader(GL_VERTEX_SHADER,VertFile);
   //  Create and compile fragment shader
   int frag = CreateShader(GL_FRAGMENT_SHADER,FragFile);
   //  Attach vertex shader
   glAttachShader(prog,vert);
   //  Attach fragment shader
   glAttachShader(prog,frag);
   //  Link program
   glLinkProgram(prog);
   //  Check for errors
   PrintProgramLog(prog);
   //  Return name
   return prog;
}

/*
 *  Start up GLUT and tell it what to do
 */
int main(int argc,char* argv[])
{
   srand(time(NULL));
   //  Initialize GLUT
   glutInit(&argc,argv);
   // Initialization function for world map and obstacle placement
   init();
   //  Request double buffered, true color window with Z buffering at 600x600
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   glutInitWindowSize(win_width,win_height);
   glutCreateWindow("Ruan Abarbanel MTBrpg MiniGame");
#ifdef USEGLEW
   //  Initialize GLEW
   if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
#endif
   //  Set callbacks
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutSpecialFunc(special);
   glutKeyboardFunc(key);
   glutIdleFunc(idle);
   //  Load textures
   texture[0] = LoadTexBMP("textures/dirt.bmp");
   texture[1] = LoadTexBMP("textures/pine2.bmp");
   texture[2] = LoadTexBMP("textures/pine_bark.bmp");
   texture[3] = LoadTexBMP("textures/rock.bmp");
   texture[4] = LoadTexBMP("textures/grass.bmp");
   texture[5] = LoadTexBMP("textures/wood_grain_2.bmp");
   texture[6] = LoadTexBMP("textures/dirt_trail.bmp");
   // texture[7] = LoadTexBMP("textures/sky.bmp");
   texture[8] = LoadTexBMP("textures/water.bmp");
   texture[9] = LoadTexBMP("textures/skyboxfront.bmp");
   texture[10] = LoadTexBMP("textures/skyboxright.bmp");
   texture[11] = LoadTexBMP("textures/skyboxback.bmp");
   texture[12] = LoadTexBMP("textures/skyboxleft.bmp");
   texture[13] = LoadTexBMP("textures/skyboxtop.bmp");
   texture[14] = LoadTexBMP("textures/skyboxbottom.bmp");

   // Shader load
   // CreateShaderProg("simple.vert","simple.frag");
   //  Pass control to GLUT so it can interact with the user
   glutMainLoop();
   fscanf(stdin, "c"); // wait for user to enter input from keyboard
   return 0;
}
