///Robert Nichita
///6/6/2018 - 6/13/2018
///Teapot Jump Game and Blender(3D modeling program) Model Loader / Renderer
//include libs
#include <windows.h>
//if on apple system, include one version of glut, otherwise, include the other version
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <glut.h>
#endif
#include <time.h>
#include <stdlib.h>
#include <cmath>
#include <queue>
#include <vector>
#include <fstream>
#include <sstream>
#include<iostream>
#include <stdio.h>
//platform object, holds an x and y coordinate
struct platform{
    float x, y;
    bool hasTea;
};
//vertex object, holds an x, y, and z coordinate
struct OBJ_Vertex{
    float x,y,z;
};
//face object, holds the IDs of 3 vertices and their associated normal vectors to create a face
struct OBJ_Face{
    float VID_1,VID_2,VID_3;
    float NID_1,NID_2,NID_3;
};

struct T_P{
    float x,y,z;
    bool dir;
};

std::string title = "Teapot Jump";//string holds the title of the game
std::string button1 = "Play";//holds the play button text
std::string button2 = "Quit";//holds the quit button text
float mouse_x,mouse_y;//holds mouse x and y coords
bool menu = true;//boolean storing the state of the menu - true = displaying menu currently - false = displaying game currently
int currentscore = 0;//current score of the player
int numsaucer = 6;//number of saucers being loaded at any given time
int platnum = numsaucer-1;//number of the previously generated platform
bool saucerloaded = false;//stores whether the model loading of the saucer was performed correctly or not, by default false, true when obj loads
bool keys[256];//stores the states of keys (held or not)
float player_x,player_y;//player x and y coordinates
float player_rot = 0;//player rotation in the z axis
float VS,HS;//vertical and horizontal speed, respectively
float aspect_ratio;//stores the current aspect ratio of the window
bool p_face_right = true;//stores whether the player is facing right or not
std::vector<OBJ_Vertex> saucer_vertices;//stores the vertices loaded from the saucer OBJ file
std::vector<OBJ_Vertex> saucer_normals;//stores the normals loaded from the saucer OBJ file
std::vector<OBJ_Face> saucer_faces;//stores the faces loaded from the saucer OBJ file
std::vector<platform> saucers;//stores the locations of the saucers
std::vector<OBJ_Vertex> teabag_vertices;//stores the vertices loaded from the saucer OBJ file
std::vector<OBJ_Vertex> teabag_normals;//stores the normals loaded from the saucer OBJ file
std::vector<OBJ_Face> teabag_faces;//stores the faces loaded from the saucer OBJ file
std::vector<T_P> teaparticles;
int particle_amt = 120;
int teacounter=0;
int teacounter2=particle_amt;

bool teabagloaded = false;

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = {0.9453125,0.9375,0.8984375, 1.0f };//except this one is modified to make the saucers look like porcelain
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8*0.9453125,0.8*0.9375,0.8*0.8984375, 1.0f };//and this one for saucer
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 80.0f };

const GLfloat teabag_light_diffuse[] = {0.90234375,0.5703125,0.3359375,1.0};
const GLfloat teabag_diffuse[] = {0.90234375,0.5703125,0.3359375,1.0};

void createTeaParticle(float playerx,float playery,bool teapot_dir)
{
    T_P tmp;

    if(teaparticles.size() >= particle_amt){
        teaparticles.erase(teaparticles.begin());
    }

    tmp.x = rand()%100 /900.0+playerx-100/1800.0;
    if(teapot_dir){
        tmp.x+=0.6;
        tmp.dir = true;
    }else{
        tmp.x-=0.6;
        tmp.dir = false;
    }
    tmp.y = rand()% 100/600.0+playery;
    tmp.z = rand()% 100/900.0-6-100/1800.0;

    teaparticles.push_back(tmp);
}

//checks collisions of the player with a given platform
bool collideWithPlatform(platform &p){
    //if the player's x and y coordinates are within a bounding box slightly under the platform, they collide with it
    if(player_x > p.x-0.65*aspect_ratio && player_x < p.x+0.65*aspect_ratio && player_y < p.y-6.0 && player_y > p.y-6.2){
        return true;
    }
    //otherwise, they don't
    return false;
}
//parses the location of vertices and normals in the respective vectors based on string input from OBJ file
void parseFace_Loc(std::vector<std::string> &vertices, std::vector<OBJ_Face> &faces){
    OBJ_Face tmp;//temporary face data storing object
    for(int i = 0; i < 3; i++)//for the 3 vertices that make up the faee
    {
        int F = vertices.at(i).find("//");//find the position of the double forward slash delimiters in the string
        int V = atoi(vertices.at(i).substr(0,F).c_str());//the vertex comes before the delimiter
        int VN = atoi(vertices.at(i).substr(F+2,vertices.at(i).size()-F-1).c_str());//the normal comes after
        //std::cout<<V<<VN<<std::endl;


        switch(i)//store ID of the vertex and normal in the appropriate part of the face object
        {
            case 0:
                tmp.VID_1 = V;
                tmp.NID_1 = VN;
                break;
            case 1:
                tmp.VID_2 = V;
                tmp.NID_2 = VN;
                break;
            case 2:
                tmp.VID_3 = V;
                tmp.NID_3 = VN;
                break;
        }
    }
    //push the face onto the faces vector of the object being loaded
    faces.push_back(tmp);
}
//generate a new platform
void genPlatform(int iter){


    platform tmp;//temporary platform storing object
    if(iter == 0){//if we are generating the first platform, place it under the player
    //std::cout<<iter<<"zero";
    tmp.x = 0;
    }else{//otherwise, randomize the x value about the middle of the screen
    //std::cout<<iter;
    tmp.x = (rand()%100)*4.0/100.0-2;
    }
    //randomize the y value based on the number of the current platform, the space between platforms must be at least 2
    tmp.y = iter*2+(rand()%15)/15;

    if(rand()%20 == 3){

        tmp.hasTea = true;
    }else{

        tmp.hasTea = false;
    }
    saucers.push_back(tmp);
}
//function to load a model from an OBJ file generated by blender using the settings "triangulate faces" and "write normals" checked
//note that the model should have relatively low polygon count or it will lag the game a lot when being rendered
bool LoadOBJ(std::string path, std::vector<OBJ_Vertex> &vertices, std::vector<OBJ_Face> &faces, std::vector<OBJ_Vertex> &normals){
    std::ifstream read(path);//access the file in an inputfilestream
    if(!read){//if the file is not accessible, throw an error
        std::cout<<"failed to load obj file: " << path << std::endl;
    }else{//otherwise load the file
    std::string line;//holds one line of the obj
        while(getline(read,line)){//for each line in the file
           // std::cout<<line<<std::endl;
            if(line.at(0) == 'v' && !(line.at(1) == 'n')){//if the first character in the line is 'v' and the second is not 'n'
                //we are loading a spatial vertex
                OBJ_Vertex temp;//create a vertex object to hold x,y,z coordinates

                std::stringstream parse(line);//stringstream used to split the string by spaces
                std::string coordinate;//resulting string is held here
                int coordcounter = 0;//counts which part of the line we're on
                while(std::getline(parse,coordinate,' ')){
                    switch(coordcounter){
                        case 0://this is the part of the line that denotes the type of data being loaded, not useful anymore
                            break;
                        case 1://this is the x coord
                            temp.x = atof(coordinate.c_str());//store the data in the temporary vertex object
                            break;
                        case 2://y coord
                            temp.y = atof(coordinate.c_str());
                            break;
                        case 3://z coord
                            temp.z = atof(coordinate.c_str());
                            break;
                    }
                    coordcounter++;//after reading one segment of the line, increment the section counter
                }
                vertices.push_back(temp);//add the vertex to the vector of vertices
            }else if(line.at(0) == 'f'){//if the first character is 'f'
                //we are loading a face
                //std::cout<<"loading a face";
                std::stringstream parser(line);//parse
                std::string vertex;
                int vertcounter = 0;
                std::vector<std::string> vertice;
                while(std::getline(parser,vertex,' ')){
                    switch(vertcounter){
                        case 0:
                          //  std::cout<<vertex<< " 0 " <<std::endl;
                            break;
                        case 1://instead of loading it into the object directly, we have to store the strings
                            vertice.push_back(vertex);
                          //  std::cout<<vertex << " 1 " <<std::endl;
                            break;
                        case 2:
                            vertice.push_back(vertex);
                           // std::cout<<vertex << " 2 " <<std::endl;
                            break;
                        case 3:
                            vertice.push_back(vertex);
                            //std::cout<<vertex << " 3 " <<std::endl;
                            parseFace_Loc(vertice, faces);//and parse them again in a separate function
                            break;
                    }
                    vertcounter++;
                }
            }else if(line.at(0) == 'v' && line.at(1) == 'n'){//if the first character of a line is 'v' and the second character is 'n'
                //we are parsing a vertex normal
                OBJ_Vertex temp;//this is exactly the same as a spatial vertex, but the x,y and z are components of a vector
                std::stringstream parses(line);
                std::string coordinate;
                int coordcounter = 0;
                while(std::getline(parses,coordinate,' ')){
                    switch(coordcounter){
                        case 0:
                            break;
                        case 1:
                            temp.x = atof(coordinate.c_str());
                            break;
                        case 2:
                            temp.y = atof(coordinate.c_str());
                            break;
                        case 3:
                            temp.z = atof(coordinate.c_str());
                            break;
                    }
                    coordcounter++;
                }
                normals.push_back(temp);


            }
        }
        return true;//return true when the model is loaded
    }
    return false;//false when the model doesn't load
}
//function to create a model loaded from an OBJ file
void createModel(std::vector<OBJ_Vertex> &vertices,std::vector<OBJ_Vertex> &normals, std::vector<OBJ_Face> &faces){
    for(int i = 0; i < faces.size();i++){//for all the faces in the model
        glBegin(GL_TRIANGLES);//draw a triangle to represent the face
        //attribute the first normal
        //std::cout<<faces.size();
        //std::cout<<"1";
        glNormal3f(normals.at(faces.at(i).NID_1-1).x, normals.at(faces.at(i).NID_1-1).y, normals.at(faces.at(i).NID_1-1).z);
        //draw the first vertex
        //std::cout<<"2";
        glVertex3f(vertices.at(faces.at(i).VID_1-1).x,vertices.at(faces.at(i).VID_1-1).y,vertices.at(faces.at(i).VID_1-1).z);
        //second normal
        //std::cout<<"3";
        glNormal3f(normals.at(faces.at(i).NID_2-1).x, normals.at(faces.at(i).NID_2-1).y, normals.at(faces.at(i).NID_2-1).z);
        //second vertex
        //std::cout<<"4";
        glVertex3f(vertices.at(faces.at(i).VID_2-1).x,vertices.at(faces.at(i).VID_2-1).y,vertices.at(faces.at(i).VID_2-1).z);
        //third normal
        //std::cout<<"5";
        glNormal3f(normals.at(faces.at(i).NID_3-1).x, normals.at(faces.at(i).NID_3-1).y, normals.at(faces.at(i).NID_3-1).z);
        //third vertex
        //std::cout<<"6"<<std::endl;
        glVertex3f(vertices.at(faces.at(i).VID_3-1).x,vertices.at(faces.at(i).VID_3-1).y,vertices.at(faces.at(i).VID_3-1).z);
        glEnd();//finish drawing this face
    }

}
//modified the resize function to force the window to 700x900 pixels
static void resize(int width, int height)
{


    const float ar = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
    aspect_ratio = glutGet(GLUT_WINDOW_WIDTH)*1.0/glutGet(GLUT_WINDOW_HEIGHT)*1.0;
    //lock window size
    glutReshapeWindow(700,900);
}

//function to draw a string at x,y using a specified font
void drawString(float x,float y,std::string word, void* font)
{
        glRasterPos3f(x,y,-6);
        for(int i = 0; i < word.size(); i++){
            glutBitmapCharacter(font,word.at(i));
        }
}
//display function
static void display(void)
{
    const double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    const double a = t*90.0;
    if(menu){//if the menu is being displayed
        //clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_COLOR);//enable color
        glDisable(GL_LIGHTING);//disable shading


        glPushMatrix();//draw all the text
        drawString(-0.5*aspect_ratio,2.5,title,GLUT_BITMAP_TIMES_ROMAN_24);//title
        glColor3b(0,0,0);
        drawString(-0.5*aspect_ratio,0.0,button1,GLUT_BITMAP_TIMES_ROMAN_24);//button 1
        drawString(-0.5*aspect_ratio,-0.5,button2,GLUT_BITMAP_TIMES_ROMAN_24);//button 2
        glPopMatrix();
        glColor3f(1,0,0);//change the color for the buttons

        glBegin(GL_POLYGON);//draw the play button
        glVertex3f(-aspect_ratio,0.2,-6);
        glVertex3f(-aspect_ratio,-0.2,-6);
        glVertex3f(aspect_ratio,-0.2,-6);
        glVertex3f(aspect_ratio,0.2,-6);
        glEnd();

        glBegin(GL_POLYGON);//draw the quit button
        glVertex3f(-aspect_ratio,0.-0.3,-6);
        glVertex3f(-aspect_ratio,-0.7,-6);
        glVertex3f(aspect_ratio,-0.7,-6);
        glVertex3f(aspect_ratio,-0.3,-6);
        glEnd();

        glDisable(GL_COLOR);//disable color
        glEnable(GL_LIGHTING);//enable shading
        glutSwapBuffers();//swap the buffers to show the drawn menu
    }else{
        //clear depth and color buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(keys['a']){//if a is pressed, move left
            p_face_right = false;
            if(HS> -0.002 && player_x > -2 * aspect_ratio){
                HS-=0.02;
            }
            HS-=0.0007;
        }
        else if(keys['d']){//if d is pressed, move right
            p_face_right = true;
             if(HS< 0.002 && player_x < 2 * aspect_ratio){
                HS+=0.02;
            }
            HS+=0.0007;
        }else{//otherwise, slow down the player
            HS+= (0-HS)/30;
        }
        if(player_x < -2*aspect_ratio && HS<0 || player_x > 2*aspect_ratio && HS>0){
            HS = 0;
        }
        VS-=0.014*0.1-0.00003*VS;//slowly cause the player to fall all the time, create a terminal velocity
        player_x +=HS;//move the player horizontally and vertically by the respective speed value each frame
        player_y +=VS;
        //each tick, create a new tea particle based on the players position
        createTeaParticle(player_x,player_y,p_face_right);
        //this determines which tea particles are shown
        if(VS>0.119&& teacounter<particle_amt-1 && teacounter < teaparticles.size()-1){//if we are moving fast enough
            teacounter++;//particle counter
            teacounter2 = particle_amt;//particle
        }else if (VS<0.119&& teacounter2 > teacounter){
            teacounter2--;
            if(teacounter > 0)
            {
                teacounter--;
            }

        }

        for(int i = teacounter2-1; i >=teacounter2-teacounter; i--){

            glPushMatrix();
                glMaterialfv(GL_FRONT,GL_DIFFUSE,teabag_diffuse);
                glLightfv(GL_LIGHT0,GL_DIFFUSE,teabag_light_diffuse);
                if(teaparticles.at(i).dir){
                    glTranslatef(teaparticles.at(i).x+(particle_amt-i)*0.01-0.02,-pow(i/2-54,2)*0.001+0.2,teaparticles.at(i).z+0.4);

                }else{
                    glTranslatef(teaparticles.at(i).x-(particle_amt-i)*0.01+0.02,-pow(i/2-54,2)*0.001+0.2,teaparticles.at(i).z+0.4);
                }
                glScalef(0.7,1.2,0.3);
                glutSolidSphere(0.022,15,15);
            glPopMatrix();
            glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
            glLightfv(GL_LIGHT0,GL_DIFFUSE,light_diffuse);
            //std::cout<<teaparticles.at(i).y;

        }
        //if the player collides with the side of the screen, stop them

        //rotate in the direction the player is facing, when approaching the end of a rotation, snap to the final position
        if(p_face_right){
            if(abs(player_rot) < 6){
                player_rot = 0;
            }
            player_rot += (0-player_rot)/3;
            //std::cout<<player_rot<<std::endl;
        }else{
            if(abs(-180-player_rot)<6){
                player_rot = -180;
            }
            player_rot += (-180-player_rot)/3;
            //std::cout<<player_rot<<std::endl;
        }

        //draw the player
        glPushMatrix();
            glTranslatef(player_x,0,-6);
            glRotatef(30,1,0,0);
            glRotatef(player_rot,0,1,0);

            glutSolidTeapot(0.4);
        glPopMatrix();

        //draw the saucers
        for(int i = 0; i < numsaucer; i++)
        {

            //if the player collides with a given saucer, they are propelled upwards
            if(collideWithPlatform(saucers.at(i)))
            {
                //player_y = saucers.at(i).y-5.8;
                if(saucers.at(i).hasTea){
                    VS = 0.5;
                }else if (VS < 0.12){
                    VS=0.12;
                }


            }
            //if the current saucer is offscreen, delete it and genereate a new one at the top
            if(saucers.at(i).y-5-player_y<-6.3)
            {
                saucers.erase(saucers.begin()+i);
                platnum++;
                //std::cout<<platnum;
                genPlatform(platnum);
            }
            //std::cout<<saucers.at(0).y<<std::endl;
            //draw the current saucer
            glPushMatrix();
                glRotatef(30,1,0,0);
                glTranslatef(saucers.at(i).x,saucers.at(i).y-9.5-player_y,-6);


                if(saucerloaded)//if the saucer loads correctly, create the model
                {
                    glScalef(0.2,0.2,0.2);
                    createModel(saucer_vertices,saucer_normals,saucer_faces);
                    if(saucers.at(i).hasTea && teabagloaded){//if the saucer has a teabag on it

                            glMaterialfv(GL_FRONT,GL_DIFFUSE,teabag_diffuse);//change the material
                            glLightfv(GL_LIGHT0,GL_DIFFUSE,teabag_light_diffuse);//change the lighting
                            glTranslatef(0,1.1+sin(a/15)/3,0);//translate the bag up, and make it bob up and down based on elapsed time
                            glRotatef(a,0,1,0);
                            glScalef(0.7,0.7,0.7);
                            createModel(teabag_vertices,teabag_normals,teabag_faces);


                        glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
                        glLightfv(GL_LIGHT0,GL_DIFFUSE,light_diffuse);
                    }

                }else{//otherwise draw a rectangular prism in its place

                    glScalef(1.0,0.2,1.0);
                    glutSolidCube(1);
                }
            glPopMatrix();

        }
        //the current score is 50* the y coordinate
        int score = player_y*50;
        currentscore = std::max(currentscore,score);//the currently displayed score is the max of the previous score and the current score
        std::stringstream conv;//convert the score to a string
        conv<<currentscore;
        std::string scorestring = conv.str();
        //std::cout<<scorestring<<std::endl;


        glPushMatrix();//draw the score
        drawString(-2,2,scorestring,GLUT_BITMAP_TIMES_ROMAN_24);
        //glRasterPos3f(-2,2,-6);
        //for(int i = 0; i < scorestring.size(); i++){
         //   glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,scorestring.at(i));
        //}
        glPopMatrix();
        glutSwapBuffers();//swap the buffers to draw the game frame
        if(player_y < saucers.at(0).y-9.5){//if the player falls off the screen
            //enable the main menu
            menu = true;
            //reset the game state
            player_y = 0;
            player_x = 0;
            currentscore = 0;
            platnum = numsaucer-1;
            saucers.clear();
            HS = 0;
            VS = 0;

            for(int i = 0; i < numsaucer;i++)
            {
                genPlatform(i);
            }
        }
    }
}

//keyboard function
static void key(unsigned char key, int x, int y)
{
    keys[key] = true;//set the pressed key to true
}
//keyboard release function
static void keyup(unsigned char key, int x, int y)
{
    keys[key] = false;//set the released key to false
}
//mouse function
static void click(int button, int state, int x, int y)
{
    //if the mouse is within the play button, and the left mouse buttno is pressed, hide the menu
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && mouse_x > 235 && mouse_x < 470 && mouse_y > 420 && mouse_y < 480){
        menu = false;
    }
    //if the mouse is within the quit button, and the left mouse button is pressed, quit the game successfully
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && mouse_x > 235 && mouse_x < 470&&mouse_y < 555 && mouse_y > 495){
        exit(EXIT_SUCCESS);
    }
}
//passive record of the mouse's coordinates
static void mouse(int x, int y)
{
    mouse_x = x;
    mouse_y = y;
}
//when nothing is happening, render the next frame
static void idle(void)
{
    glutPostRedisplay();
}
//standard shader constants provided in the example




/* Program entry point */

int main(int argc, char *argv[])
{
    //init randomizer
    srand(time(NULL));
    //init glut
    glutInit(&argc, argv);
    //window size 700x900
    glutInitWindowSize(700,900);
    //window position top left corner
    glutInitWindowPosition(0,0);
    //display mode with RGB buffer, depth buffer, double buffering
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    for(int i = 0; i < particle_amt; i++){
        createTeaParticle(0,0,true);
    }

    //attempt to load the saucer model, note whether it loaded properly or not
    if(LoadOBJ("C:\\saucer.obj",saucer_vertices,saucer_faces,saucer_normals))
    {
        saucerloaded = true;
    }else{
        saucerloaded = false;
        numsaucer = 2000;//if the saucer isnt loaded and we're loading rectangular prisms, we have the liberty to create many more platforms
        platnum = numsaucer-1;
    }
    //attempt to load the teabag object
    if(LoadOBJ("C:\\teabag.obj",teabag_vertices,teabag_faces,teabag_normals)){
        teabagloaded = true;
    }

    //generate the first set of platforms
    for(int i = 0; i < numsaucer; i++){
        genPlatform(i);//generate the platforms initially
    }
    //debugging stuff
    for(int i = 0; i < saucer_faces.size() ;i++){
       // std::cout<<saucer_faces.at(i).NID_1<<std::endl;
    }
    /*if(saucerloaded){
        for( int i = 0; i < saucer_vertices.size(); i++){
            std::cout<<"V: "<<i<<" x: "<< saucer_vertices.at(i).x<< " y: "<< saucer_vertices.at(i).y<< " z: " << saucer_vertices.at(i).z<<std::endl;
        }
        for(int i = 0; i < saucer_faces.size();i++){
            std::cout<<"F: "<<i<<" ID1: " << saucer_faces.at(i).ID_1<< " ID2: " << saucer_faces.at(i).ID_2 << " ID3: "<<saucer_faces.at(i).ID_3<<std::endl;
        }
    }*/
    //create the window
    glutCreateWindow("GLUT Shapes");
    //init pointers to various event-driven functions
    glutReshapeFunc(resize);//resize
    glutDisplayFunc(display);//repaint
    glutKeyboardFunc(key);//key press
    glutKeyboardUpFunc(keyup);//key release
    glutMouseFunc(click);//mouse click
    glutPassiveMotionFunc(mouse);//mouse x/y record
    glutIdleFunc(idle);//nothing else happening
    glutIgnoreKeyRepeat(true);//ignore the repeated inputs from a key once it is held down
    glClearColor(1,1,1,1);//set the clear color to white

    glEnable(GL_DEPTH_TEST);//enable depth test
    glDepthFunc(GL_LESS);//depth test for fragements less than the depth buffer (i.e. towards us)

    glEnable(GL_LIGHT0);//enable the first light
    glEnable(GL_NORMALIZE);//normalize light vectors to a length of 1
    glEnable(GL_LIGHTING);//enable lighting
    //set light constants i.e. ambient, diffuse, specular rgb and position
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    //set the reflective properties of the material and shininess
    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
    //store the initial aspect ratio
    aspect_ratio = glutGet(GLUT_WINDOW_WIDTH)*1.0/glutGet(GLUT_WINDOW_HEIGHT)*1.0;
    //begin the main glut loop
    glutMainLoop();

    return EXIT_SUCCESS;
}
