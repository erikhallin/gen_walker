#include <windows.h>
#include <vector>
#include <ctime>
#include <stdlib.h>
#include <gl/gl.h>
#include <string>
#include <sstream>

using namespace std;

int _w_width=600;
int _w_height=400;
int _mutation_rate=10;
int _attack_range=1;
int _spawn_cost=1;
int _spawn_count_buffer=20;
int _max_walkers=5000;
int _min_walkers=500;
int _lifetime_def=3000;

struct st_marker
{
    int x,y;
    float r,g,b;
    int lifetime;
    st_marker(int _x,int _y,float _r,float _g,float _b,int _lifetime)
    {
        x=_x; y=_y;
        r=_r; g=_g; b=_b;
        lifetime=_lifetime;
    }
};

enum e_ground_types
{
    gt_empty=0,
    gt_food,
    gt_blood,
};

struct st_walker
{
    int x,y;
    int curr_seq_pos;
    int material;
    int lifeleft;
    string seq;
    bool to_be_removed=false;

    st_walker(int _x,int _y,string _seq)
    {
        x=_x;
        y=_y;
        seq=_seq;
        curr_seq_pos=0;
        material=0;
        lifeleft=_lifetime_def+rand()%_lifetime_def-_lifetime_def*0.5;
    }
};


vector< vector<int> > g_vec_board;
vector<st_walker> g_vec_walkers;
vector<st_marker> g_vec_markers;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);
bool init(void);
bool update(void);
bool draw(void);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GeneticWalker";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);


    if (!RegisterClassEx(&wcex))
        return 0;

    if(true)//if fullscreen
    {
        //Detect screen resolution
        RECT desktop;
        // Get a handle to the desktop window
        const HWND hDesktop = GetDesktopWindow();
        // Get the size of screen to the variable desktop
        GetWindowRect(hDesktop, &desktop);
        // The top left corner will have coordinates (0,0)
        // and the bottom right corner will have coordinates
        // (horizontal, vertical)
        _w_width  = desktop.right;
        _w_height = desktop.bottom;
    }

    hwnd = CreateWindowEx(0,
                          "GeneticWalker",
                          "GeneticWalker",
                          WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          _w_width,
                          _w_height,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    EnableOpenGL(hwnd, &hDC, &hRC);

    //startup
    if(!init())
    {
        //cout<<"ERROR: Could not init\n";
        return 1;
    }

    while (!bQuit)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            update();
            draw();
            SwapBuffers(hDC);
        }
    }

    DisableOpenGL(hwnd, hDC, hRC);
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    *hDC = GetDC(hwnd);

    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);

    //set 2D mode
    glClearColor(0.0,0.0,0.0,0.0);  //Set the cleared screen colour to black
    glViewport(0,0,_w_width,_w_height);   //This sets up the viewport so that the coordinates (0, 0) are at the top left of the window

    //Set up the orthographic projection so that coordinates (0, 0) are in the top left
    //and the minimum and maximum depth is -10 and 10. To enable depth just put in
    //glEnable(GL_DEPTH_TEST)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,_w_width,_w_height,0,-1,1);

    //Back to the modelview so we can draw stuff
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /*//Enable antialiasing
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearStencil(0);*/
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

bool init()
{
    srand(time(0));
    for(int x=0;x<_w_width;x++)
    {
        g_vec_board.push_back(vector<int>());
        for(int y=0;y<_w_height;y++)
        {
            g_vec_board[x].push_back(gt_empty);
        }
    }

    //spawn walkers
    int walkers_to_spawn=_min_walkers;
    for(int i=0;i<walkers_to_spawn;i++)
    {
        int xpos=rand()%_w_width;
        int ypos=rand()%_w_height;
        int seq_length=rand()%100+4;
        string seq;
        for(int j=0;j<seq_length;j++)
        {
            if(rand()%2==0)
                seq.append(1,'0');
            else
                seq.append(1,'1');
        }

        g_vec_walkers.push_back(st_walker(xpos,ypos,seq));
    }

    return true;
}

bool update()
{
    //update walkers
    for(int i=0;i<(int)g_vec_walkers.size();i++)
    {
        //check life
        g_vec_walkers[i].lifeleft-=1;
        if(g_vec_walkers[i].lifeleft<0)
        {
            g_vec_walkers[i].to_be_removed=true;
            continue;
        }

        //read seq
        int seq_pos=g_vec_walkers[i].curr_seq_pos;
        if(seq_pos+4>(int)g_vec_walkers[i].seq.length())
        {
            //restart seq
            g_vec_walkers[i].curr_seq_pos=0;
            seq_pos=0;
        }
        string seq_bit=g_vec_walkers[i].seq.substr(seq_pos,4);
        int action=0;
        stringstream ss(seq_bit);
        ss>>action;
        g_vec_walkers[i].curr_seq_pos+=4;

        //make action
        switch(action)
        {
            //move
            case 0://up
            {
                g_vec_walkers[i].y-=1;
                if(g_vec_walkers[i].y<0) g_vec_walkers[i].y=_w_height-1;
            }break;
            case 10://left
            {
                g_vec_walkers[i].x-=1;
                if(g_vec_walkers[i].x<0) g_vec_walkers[i].x=_w_width-1;
            }break;
            case 100://down
            {
                g_vec_walkers[i].y+=1;
                if(g_vec_walkers[i].y>=_w_height) g_vec_walkers[i].y=0;
            }break;
            case 1000://right
            {
                g_vec_walkers[i].x+=1;
                if(g_vec_walkers[i].x>=_w_width) g_vec_walkers[i].x=0;
            }break;

            //food
            /*case 1://plant
            {
                if(g_vec_walkers[i].x<0 || g_vec_walkers[i].x>=_w_width ||
                   g_vec_walkers[i].y<0 || g_vec_walkers[i].y>=_w_height) ;//skip
                else g_vec_board[g_vec_walkers[i].x][g_vec_walkers[i].y]=gt_food;
            }break;*/
            case 11://eat
            {
                if(g_vec_board[g_vec_walkers[i].x][g_vec_walkers[i].y]==gt_food)
                {
                    g_vec_board[g_vec_walkers[i].x][g_vec_walkers[i].y]=gt_empty;
                    g_vec_walkers[i].material+=1;
                }
            }break;

            //spawn
            case 111:
            {
                if((int)g_vec_walkers.size()<_max_walkers+_spawn_count_buffer)
                if(g_vec_walkers[i].material>=_spawn_cost)
                if(rand()%10<=g_vec_walkers[i].material)
                {
                    g_vec_walkers[i].material=0;
                    //spawn new walker
                    int xpos=g_vec_walkers[i].x;
                    int ypos=g_vec_walkers[i].y;
                    string seq=g_vec_walkers[i].seq;
                    for(int j=0;j<(int)seq.length();j++)
                    {
                        if(rand()%_mutation_rate==0)
                        {
                            if(seq[j]=='0') seq[j]='1';
                            else            seq[j]='0';
                        }
                    }
                    //extend seq
                    int change_size=rand()%11-5;
                    if(change_size>0)
                    {
                        for(int j=0;j<change_size;j++)
                        {
                            if(rand()%2==0)
                                seq.append(1,'0');
                            else
                                seq.append(1,'1');
                        }
                    }
                    else if(change_size<0 && (int)seq.length()-change_size>4)
                    {
                        change_size*=-1;
                        seq.erase(seq.begin()+(int)seq.length()-change_size);
                    }

                    g_vec_walkers.push_back(st_walker(xpos,ypos,seq));
                }
            }break;

            //attack
            case 1111:
            {
                for(int j=0;j<(int)g_vec_walkers.size();j++)
                {
                    if(i==j) continue;
                    if(g_vec_walkers[j].x+_attack_range>g_vec_walkers[i].x &&
                       g_vec_walkers[j].x-_attack_range<g_vec_walkers[i].x &&
                       g_vec_walkers[j].y+_attack_range>g_vec_walkers[i].y &&
                       g_vec_walkers[j].y-_attack_range<g_vec_walkers[i].y)
                    {
                        g_vec_walkers[j].to_be_removed=true;
                        g_vec_walkers[i].material+=10;

                        //make mark on ground
                        g_vec_markers.push_back(st_marker(g_vec_walkers[j].x,g_vec_walkers[j].y,1,0,0,1000));
                    }
                }
            }break;
        }


    }

    //remove the dead
    for(int i=0;i<(int)g_vec_walkers.size();i++)
    {
        if(g_vec_walkers[i].to_be_removed)
        {
            //remove walker
            g_vec_walkers.erase(g_vec_walkers.begin()+i);
            i--;
        }
    }

    //spawn new walkers
    while((int)g_vec_walkers.size()<_min_walkers)
    {
        int xpos=rand()%_w_width;
        int ypos=rand()%_w_height;
        int seq_length=rand()%100+4;
        string seq;
        for(int j=0;j<seq_length;j++)
        {
            if(rand()%2==0)
                seq.append(1,'0');
            else
                seq.append(1,'1');
        }

        g_vec_walkers.push_back(st_walker(xpos,ypos,seq));
    }

    //update markers
    for(int i=0;i<(int)g_vec_markers.size();i++)
    {
        g_vec_markers[i].lifetime-=1;
        if(g_vec_markers[i].lifetime<0)
        {
            g_vec_markers.erase(g_vec_markers.begin()+i);
            i--;
        }
    }

    return true;
}

bool draw()
{
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    glPushMatrix();

    //draw board
    /*glColor3f(1,1,1);
    glBegin(GL_POINTS);
    for(int x=0;x<_w_width;x++)
    {
        for(int y=0;y<_w_height;y++)
        {
            switch(g_vec_board[x][y])
            {
                case gt_food:
                {
                    glColor3f(0,1,0);
                    glVertex2f(x,y);
                }break;
                case gt_blood:
                {
                    glColor3f(1,0,0);
                    glVertex2f(x,y);
                }break;
            }
        }
    }
    glEnd();*/

    //draw markers
    glBegin(GL_POINTS);
    for(int i=0;i<(int)g_vec_markers.size();i++)
    {
        glColor3f(g_vec_markers[i].r,g_vec_markers[i].g,g_vec_markers[i].b);
        glVertex2f(g_vec_markers[i].x,g_vec_markers[i].y);
    }
    glEnd();

    //draw walkers
    glColor3f(1,1,1);
    glBegin(GL_POINTS);
    for(int i=0;i<(int)g_vec_walkers.size();i++)
    {
        glVertex2f(g_vec_walkers[i].x,g_vec_walkers[i].y);
    }
    glEnd();

    glPopMatrix();

    return true;
}



