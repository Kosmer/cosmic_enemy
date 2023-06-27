#include <iostream>
#include "ShaderMaker.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "HUD_Logger.h"
#include <GL/glut.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static unsigned int programId;
#define PI 3.14159265358979323846
#define numTentacles 6
#define boss_life 100
#define spaceship_life 20

unsigned int VAO, VAO_SPACE, VAO_TENTACOLO, VAO_BOSS;
unsigned int VBO, VBO_C, VBO_N, loc, MatProj, MatModel;

using namespace glm;

vec4 col_bianco =	{ 1.0,1.0,1.0, 1.0 };
vec4 col_rosso =	{ 1.0,0.0,0.0, 1.0 };
vec4 col_rosso_scuro = { 0.8,0.0,0.0, 1.0 };
vec4 col_nero =		{ 0.0,0.0,0.0, 1.0 };
vec4 col_magenta =	{ 1.0,1.0,0.0, 1.0 };
vec4 col_verde =    { 0.0,1.0,0.0, 1.0 };
vec4 col_verde_scuro = { 0.0,0.45,0.0, 1.0 };

mat4 Projection;  //Matrice di proiezione
mat4 Model; //Matrice per il cambiamento da Sistema diriferimento dell'oggetto OCS a sistema Mondo WCS
typedef struct { float x, y, r, g, b, a; } Point;

int nTriangles = 30;
int nVertices_Navicella = 12 * nTriangles + 1;

int nvBocca = 7;
int nVertices_Tentacolo = 2*3 * nTriangles;
int nVertices_Boss = 3 * nTriangles + 4 * 3 * nTriangles + nvBocca;

Point* Navicella = new Point[nVertices_Navicella];

int vertices_space = 10;
Point* Space = new Point[vertices_space];

// Viewport size
static int width = 1440;
static int height = 900;
GLfloat aspect_ratio = 16.0f / 9.0f;

float posx_Proiettile = 0, posy_Proiettile = 0;

//ANIMARE V
float VelocitaOrizzontale = 0; //velocita orizzontale (pixel per frame)
float maxSpeed = 20;
int scuoti = 0;
float posx = width / 2.0; //coordinate sul piano della posizione iniziale della navicella
float posy = height * 0.2;
float posizione_di_equilibrio = posy;
float angolo = 0;

bool pressing_left = false;
bool pressing_right = false;
bool pressing_attack = false;

float angoloV = 0;
double range_fluttuazione = 3; // fluttuazione su-giù 
double angle = 0;				// angolo di fluttuazione
double angle_offset = 0.5; // quanto   accentuata è l'oscillazione angolare
double float_yoffset = 0; // distacco dalla posizione d'equilibrio 
int frame_animazione = 0; // usato per animare la fluttuazione
int frame = 0;
bool start = 0;

//remake

float passo_Nemici = ((float)width) / numTentacles;
float gapTentacoli = height / numTentacles;

bool gameStatus = 0; // 0= in corso; 1= vittoria; 2= sconfitta

float posxT[numTentacles];
float posyT[numTentacles];
float posyB = height - 50;
float posxB = width / 2;

double range_mov_tentacoli_y = height/2;
double range_mov_tentacoli_x = width / (numTentacles*2);
double range_mov_boss = width/2;
float frame_animazione_boss = 0; // usato per animare la fluttuazione del boss
float frame_animazione_tentacoli_y[numTentacles]; // usato per animare la fluttuazione dei tentacoli
float frame_animazione_tentacoli_x[numTentacles];
double float_yoffset_boss = width / 2; // distacco dalla posizione d'equilibrio del boss
double float_yoffset_tentacoli = height*1.5; // distacco dalla posizione d'equilibrio dei tentacoli
double float_xoffset_tentacoli[numTentacles];

int bossPhase = 0;
int* tentacleLives;
int bossLife = boss_life;
int tentacleLife = 20;
int spaceshipLife = spaceship_life;

Point* Tentacolo = new Point[nVertices_Tentacolo];
Point* Boss = new Point[nVertices_Boss];

float lerp(float a, float b, float t) {
	//Interpolazione lineare tra a e b secondo amount
	return (1 - t) * a + t * b;
}

double  degtorad(double angle) {
	return angle * PI / 180;
}

void updateTentacoli(int value)
{
	for (int i = 0;i < numTentacles;i++) {
		if (float_yoffset_tentacoli >= 0) {
			if (bossPhase == 0) {
				frame_animazione_tentacoli_y[i] += 0.5;
			}
			else if (bossPhase == 1) {
				frame_animazione_tentacoli_y[i] += 1;
			}
			else if (bossPhase == 2) {
				frame_animazione_tentacoli_y[i] += 2;
			}
			if (frame_animazione_tentacoli_y[i] >= 360) {
				frame_animazione_tentacoli_y[i] -= 360;
			}
		}
		if (float_xoffset_tentacoli[i] >= 0) {
			if (bossPhase == 0) {
				frame_animazione_tentacoli_x[i] += 0.5;
			}
			else if (bossPhase == 1) {
				frame_animazione_tentacoli_x[i] += 1;
			}
			else if (bossPhase == 2) {
				frame_animazione_tentacoli_x[i] += 2;
			}
			if (frame_animazione_tentacoli_x[i] >= 360) {
				frame_animazione_tentacoli_x[i] -= 360;
			}
		}
		posyT[i] = float_yoffset_tentacoli + sin(degtorad(frame_animazione_tentacoli_y[i])) * range_mov_tentacoli_y;
		posxT[i] = float_xoffset_tentacoli[i] + sin(degtorad(frame_animazione_tentacoli_x[i])) * range_mov_tentacoli_x;
	}

	glutPostRedisplay();
	glutTimerFunc(15, updateTentacoli, 0);
}

void updateBoss(int value) {
	if (float_yoffset_boss >= 0) {
		if (bossPhase == 0) {
			frame_animazione_boss += 0.5;
		}else if (bossPhase == 1) {
			frame_animazione_boss += 1;
		}else if (bossPhase == 2) {
			frame_animazione_boss += 2;
		}
		if (frame_animazione_boss >= 360) {
			frame_animazione_boss -= 360;
		}
	}

	posxB = float_yoffset_boss + sin(degtorad(frame_animazione_boss)) * range_mov_boss;

	glutPostRedisplay();
	glutTimerFunc(15, updateBoss, 0);
}

void updateP(int a)
{
	float timeValue = glutGet(GLUT_ELAPSED_TIME);
	glutPostRedisplay();
	glutTimerFunc(50, updateP, 0);
}
void updateProiettile(int value)
{
	//Ascissa del proiettile durante lo sparo
	posx_Proiettile = 0;
	//Ordinata del proettile durante lo sparo
	posy_Proiettile++;

	//L'animazione deve avvenire finchè  l'ordinata del proiettile raggiunge un certo valore fissato
	if (posy_Proiettile <= 800)
		glutTimerFunc(5, updateProiettile, 0);
	else
		posy_Proiettile = 0;

	glutPostRedisplay();
}

// Aggioramento della posizione navicella
void updateV(int a)
{
	if (gameStatus == 0) {
		bool moving = false;

		if (pressing_left)
		{
			if (VelocitaOrizzontale - 1 > -maxSpeed) {
				VelocitaOrizzontale -= 1;
			}
			moving = true;
		}
		if (pressing_right)
		{
			if (VelocitaOrizzontale + 1 < maxSpeed) {
				VelocitaOrizzontale += 1;
			}
			moving = true;
		}

		if (float_yoffset >= 0) {
			frame_animazione += 5;
			if (frame_animazione >= 360) {
				frame_animazione -= 360;
			}
		}

		if (!moving) {

			if (VelocitaOrizzontale > 0)
			{
				if (VelocitaOrizzontale - 4 > -maxSpeed) {
					VelocitaOrizzontale -= 4;
				}
				if (VelocitaOrizzontale < 0)
					VelocitaOrizzontale = 0;
			}
			if (VelocitaOrizzontale < 0)
			{
				if (VelocitaOrizzontale + 4 < maxSpeed) {
					VelocitaOrizzontale += 4;
				}
				if (VelocitaOrizzontale > 0)
					VelocitaOrizzontale = 0;
			}
		}

		angolo = cos(degtorad(frame_animazione)) * angle_offset - VelocitaOrizzontale * 0.5;
		
		//Aggioramento della posizione in x della navicella, che regola il movimento orizzontale
		posx += VelocitaOrizzontale;
		// Gestione Bordi viewport:
		// Se la navicella assume una posizione in x minore di 0 o maggiore di width dello schermo
		// facciamo rimbalzare la navicella ai bordi dello schermo
		if (posx < 0) {
			posx = 50;
			VelocitaOrizzontale = -VelocitaOrizzontale * 0.1;
		}
		if (posx > width) {
			posx = width - 50;
			VelocitaOrizzontale = -VelocitaOrizzontale * 0.1;
		}
		// calcolo y come somma dei seguenti contributi: 
		//            pos. di equilibrio + oscillazione periodica
		posy = posizione_di_equilibrio + sin(degtorad(frame_animazione)) * range_fluttuazione;


		glutPostRedisplay();
		glutTimerFunc(15, updateV, 0);
	
	}

	
}

void keyboardPressedEvent(unsigned char key, int x, int y)
{
	if (gameStatus==0) {
		switch (key)
		{
		case ' ':
			pressing_attack = true;
			updateProiettile(0);
			break;
		case 'A':
		case 'a':
			pressing_left = true;
			break;
		case 'D':
		case 'd':
			pressing_right = true;
			break;
		case 27:
			exit(0);
			break;
		default:
			break;
		}
	}
}

void keyboardReleasedEvent(unsigned char key, int x, int y)
{
	if (gameStatus==0) {
		switch (key)
		{
		case ' ':
			pressing_attack = false;
			break;
		case 'A':
		case 'a':
			pressing_left = false;
			break;
		case 'D':
		case 'd':
			pressing_right = false;
			break;
		default:
			break;
		}
	}
}

/////////////////////////// Disegna geometria scena //////////////////////////////////////

//void disegna_tentacoli(Point* Punti, vec4 color_top, vec4 color_bot)
//{
//	float alfa = 2, step=PI/4;
//	Point P0;
//	int i;
//	P0.x = 0.0;	P0.y = 0.0;  // centro circonferenza
//	int cont = 0;
//
//	for (i = 0; i < 8 ;i++)//  per ciascuno degli 8 tentacoli
//	{
//		Punti[cont].x = cos(i*step);  // punto sulla circonferenza 
//		Punti[cont].y = sin(i*step);
//		Punti[cont].r = color_bot.r; Punti[cont].g = color_bot.g; Punti[cont].b = color_bot.b; Punti[cont].a = color_bot.a;
//		Punti[cont + 1].x = lerp(P0.x, Punti[cont].x, alfa);  //punto fuori circonferenza
//		Punti[cont + 1].y = lerp(P0.y, Punti[cont].y, alfa);
//		Punti[cont + 1].r = color_top.r; Punti[cont + 1].g = color_top.g; Punti[cont + 1].b = color_top.b; Punti[cont + 1].a = color_top.a;
//		cont += 2;
//	}
//}

void disegna_piano(float x, float y, float width, float height, vec4 color_top, vec4 color_bot, Point* piano)
{
	piano[0].x = x;	piano[0].y = y; 
	piano[0].r = color_bot.r; piano[0].g = color_bot.g; piano[0].b = color_bot.b; piano[0].a = color_bot.a;
	piano[1].x = x + width;	piano[1].y = y;	
	piano[1].r = color_top.r; piano[1].g = color_top.g; piano[1].b = color_top.b; piano[1].a = color_top.a;
	piano[2].x = x + width;	piano[2].y = y + height; 
	piano[2].r = color_bot.r; piano[2].g = color_bot.g; piano[2].b = color_bot.b; piano[2].a = color_bot.a;

	piano[3].x = x + width;	piano[3].y = y + height; 
	piano[3].r = color_bot.r; piano[3].g = color_bot.g; piano[3].b = color_bot.b; piano[3].a = color_bot.a;
	piano[4].x = x;	piano[4].y = y + height; 
	piano[4].r = color_top.r; piano[4].g = color_top.g; piano[4].b = color_top.b; piano[4].a = color_top.a;
	piano[5].x = x;	piano[5].y = y; 
	piano[5].r = color_bot.r; piano[5].g = color_bot.g; piano[5].b = color_bot.b; piano[5].a = color_bot.a;
}

void disegna_cerchio(float cx, float cy, float raggiox, float raggioy, vec4 color_top, vec4 color_bot, Point* Cerchio)
{
	float stepA = (2 * PI) / nTriangles;

	int comp = 0;
	for (int i = 0; i < nTriangles; i++)
	{
		Cerchio[comp].x = cx + cos((double)i * stepA) * raggiox;
		Cerchio[comp].y = cy + sin((double)i * stepA) * raggioy;
		Cerchio[comp].r = color_top.r; Cerchio[comp].g = color_top.g; Cerchio[comp].b = color_top.b; Cerchio[comp].a = color_top.a;

		Cerchio[comp + 1].x = cx + cos((double)(i + 1) * stepA) * raggiox;
		Cerchio[comp + 1].y = cy + sin((double)(i + 1) * stepA) * raggioy;
		Cerchio[comp + 1].r = color_top.r; Cerchio[comp + 1].g = color_top.g; Cerchio[comp + 1].b = color_top.b; Cerchio[comp + 1].a = color_top.a;

		Cerchio[comp + 2].x = cx;
		Cerchio[comp + 2].y = cy;
		Cerchio[comp + 2].r = color_bot.r; Cerchio[comp + 2].g = color_bot.g; Cerchio[comp + 2].b = color_bot.b; Cerchio[comp + 2].a = color_bot.a;

		comp += 3;
	}
}

void Parte_superiore_navicella(float cx, float cy, float raggiox, float raggioy, vec4 color_top, vec4 color_bot, Point* Cerchio) {
	int i;
	int comp = 0; 
	float A = PI / 4;
	float B = 3 / 4 * PI;
	// arco tra A e B dato da PI/2
	float stepA = (PI/2)  / nTriangles;

	for (i = 0; i < nTriangles ; i++)
		{
		Cerchio[comp].x = cx + cos(A+(double)i * stepA) * raggiox;
		Cerchio[comp].y = cy + sin(A+(double)i * stepA) * raggioy;
		Cerchio[comp].r = color_top.r; Cerchio[comp].g = color_top.g; Cerchio[comp].b = color_top.b; Cerchio[comp].a = color_top.a;
		Cerchio[comp + 1].x = cx + cos(A+(double)(i + 1) * stepA) * raggiox;
		Cerchio[comp + 1].y = cy + sin(A+(double)(i + 1) * stepA) * raggioy;
		Cerchio[comp + 1].r = color_top.r; Cerchio[comp + 1].g = color_top.g; Cerchio[comp + 1].b = color_top.b; Cerchio[comp + 1].a = color_top.a;
		Cerchio[comp + 2].x =cx;
		Cerchio[comp + 2].y = cy;
		Cerchio[comp + 2].r = color_bot.r; Cerchio[comp + 2].g = color_bot.g; Cerchio[comp + 2].b = color_bot.b; Cerchio[comp + 2].a = color_bot.a;
		comp += 3;
	}
}

void disegna_Navicella(vec4 color_top_Navicella, vec4 color_bot_Navicella, vec4 color_top_corpo, vec4 color_bot_corpo, vec4 color_top_Oblo, vec4 color_bot_Oblo, Point* Navicella)
{
	int cont, i, v_Oblo;
	Point* Corpo;
	int v_Corpo =3 * nTriangles;
	Corpo = new Point[v_Corpo];

	Parte_superiore_navicella(0.0, -0.2, 0.3, 0.6, color_top_Navicella, color_bot_Navicella, Navicella);

	//Costruisci Corpo   cerchio + cintura + 3 bottoni 
	disegna_cerchio(0.0, -1.0, 0.7, 1.0, color_top_corpo, color_bot_corpo, Corpo);
	cont = 3 * nTriangles;
	for (i = 0; i < 3 * nTriangles; i++)
	{
		Navicella[i + cont].x = Corpo[i].x;
		Navicella[i + cont].y = Corpo[i].y;
		Navicella[i + cont].r = Corpo[i].r;	Navicella[i + cont].g = Corpo[i].g;	Navicella[i + cont].b = Corpo[i].b;	Navicella[i + cont].a = Corpo[i].a;
	}

	//Costruisci Corpo 2
	disegna_cerchio(0.6, -1.0, 0.2, 0.6, color_top_Oblo, color_bot_Oblo, Corpo);
	cont = 6 * nTriangles;
	for (i = 0; i < 3 * nTriangles; i++)
	{
		Navicella[i + cont].x = Corpo[i].x;
		Navicella[i + cont].y = Corpo[i].y;
		Navicella[i + cont].r = Corpo[i].r;	Navicella[i + cont].g = Corpo[i].g;	Navicella[i + cont].b = Corpo[i].b;	Navicella[i + cont].a = Corpo[i].a;
	}
    //Costruisci Corpo 3
	disegna_cerchio(-0.6, -1.0, 0.2, 0.6, color_top_Oblo, color_bot_Oblo, Corpo);
	cont = 9 * nTriangles;
	for (i = 0; i < 3 * nTriangles; i++)
	{
		Navicella[i + cont].x = Corpo[i].x;
		Navicella[i + cont].y = Corpo[i].y;
		Navicella[i + cont].r = Corpo[i].r;	Navicella[i + cont].g = Corpo[i].g;	Navicella[i + cont].b = Corpo[i].b;	Navicella[i + cont].a = Corpo[i].a;
	}
	cont = 12 * nTriangles;

//Proiettile
	Navicella[cont].x = 0;
	Navicella[cont].y = 0;
	Navicella[cont].r = 0;	Navicella[cont].g = 1;	Navicella[cont].b = 0.9;	Navicella[cont].a = 0.7;
}

void disegna_tentacoli(vec4 color_top_Tentacolo, vec4 color_bot_Tentacolo, Point* Tentacolo)
{
	int i, cont;
	int v_faccia = 3 * nTriangles;
	Point* Occhio = new Point[v_faccia];

	// Disegna corpo del Tentacolo
	disegna_cerchio(0.0, 0.0, 0.7, 27.0, color_top_Tentacolo, color_bot_Tentacolo, Tentacolo);

}

void disegna_boss(vec4 color_top_Boss, vec4 color_bot_Boss, vec4 color_top_Occhio, vec4 color_bot_Occhio, Point* Boss)
{
	int i, cont;
	int v_faccia = 3 * nTriangles;
	Point* Occhio = new Point[v_faccia];

	// Disegna faccia del boss
	disegna_cerchio(0.0, 0.0, 10.0, 8.0, color_top_Boss, color_bot_Boss, Boss);

	// Disegna i quattro occhi
	disegna_cerchio(-7.0, 0.5, 2.0, 1.5, color_top_Occhio, color_bot_Occhio, Occhio);
	cont = v_faccia;
	for (i = 0; i < v_faccia; i++)
		Boss[i +cont] = Occhio[i];
	disegna_cerchio(7.0, 0.5, 2.0, 1.5, color_top_Occhio, color_bot_Occhio, Occhio);
	cont = cont + v_faccia;
	for (i = 0; i < v_faccia; i++)
		Boss[i + cont] = Occhio[i];
	disegna_cerchio(-4.0, -1.5, 0.8, 1.0, color_top_Occhio, color_bot_Occhio, Occhio);
	cont = cont + v_faccia;
	for (i = 0; i < v_faccia; i++)
		Boss[i + cont] = Occhio[i];
	disegna_cerchio(4.0, -1.5, 0.8, 1.0, color_top_Occhio, color_bot_Occhio, Occhio);
	cont = cont + v_faccia;
	for (i = 0; i < v_faccia; i++)
		Boss[i + cont] = Occhio[i];

	cont = cont + v_faccia;

	//Aggiungi bocca
	Boss[cont].x = -5.5;	Boss[cont].y = -6.0;
	Boss[cont].r = col_nero.r;Boss[cont].g = col_nero.g;Boss[cont].b = col_nero.b;Boss[cont].a = col_nero.a;

	Boss[cont+1].x = -4.5;	Boss[cont].y = -5.5;
	Boss[cont+1].r = col_nero.r;Boss[cont].g = col_nero.g;Boss[cont].b = col_nero.b;Boss[cont].a = col_nero.a;

	Boss[cont + 2].x = -2.25;	Boss[cont + 1].y = -4.0;
	Boss[cont + 2].r = col_nero.r; Boss[cont + 1].g = col_nero.g; Boss[cont + 1].b = col_nero.b; Boss[cont + 1].a = col_nero.a;

	Boss[cont + 3].x = 0.0;	Boss[cont + 2].y = -5.5;
	Boss[cont + 3].r = col_nero.r; Boss[cont + 2].g = col_nero.g; Boss[cont + 2].b = col_nero.b; Boss[cont + 2].a = col_nero.a;

	Boss[cont + 4].x = 2.25;	Boss[cont + 3].y = -4.0;
	Boss[cont + 4].r = col_nero.r; Boss[cont + 3].g = col_nero.g; Boss[cont + 3].b = col_nero.b; Boss[cont + 3].a = col_nero.a;

	Boss[cont + 5].x = 4.5;Boss[cont + 4].y = -5.5;
	Boss[cont + 5].r = col_nero.r; Boss[cont + 4].g = col_nero.g; Boss[cont + 4].b = col_nero.b; Boss[cont + 4].a = col_nero.a;

	Boss[cont+6].x = -5.5;	Boss[cont].y = -6.0;
	Boss[cont+6].r = col_nero.r;Boss[cont].g = col_nero.g;Boss[cont].b = col_nero.b;Boss[cont].a = col_nero.a;

	cont = cont + 7;	
}

void initShader(void)
{
	GLenum ErrorCheckValue = glGetError();

	char* vertexShader = (char*)"vertexShader_C_M.glsl";
	char* fragmentShader = (char*)"fragmentShader_C_M.glsl";
	programId = ShaderMaker::createProgram(vertexShader, fragmentShader);
	glUseProgram(programId);
}

void init(void)
{
	//Disegno SPAZIO
	vec4 col_top = {0.1,0.1,0.4, 0.7};
	disegna_piano(0.0, 0.0, width, height, col_nero, col_top, Space);
	glGenVertexArrays(1, &VAO_SPACE);
	glBindVertexArray(VAO_SPACE);
	glGenBuffers(1, &VBO_C);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_C);
	glBufferData(GL_ARRAY_BUFFER, vertices_space * sizeof(Point), &Space[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	
	//Disegno NAVICELLA
	vec4 color_top_Navicella = { 0.0,0.0,0.0,1.0 };
	vec4 color_bot_Navicella = { 0.7,0.0,0.0,1.0 };
	vec4 color_top_Corpo     = { 0.0,0.0,0.0,1.0 };
	vec4 color_bot_Corpo     = { 0.0,0.0,0.7,1.0 };
	vec4 color_top_Oblo      = { 0.0,0.0,0.0,0.5 };
	vec4 color_bot_Oblo      = { 1.0,1.0,1.0,0.8 };
	
	disegna_Navicella(color_top_Navicella, color_bot_Navicella, color_top_Corpo, color_bot_Corpo, color_top_Oblo, color_bot_Oblo, Navicella);
	//Genero un VAO navicella
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, nVertices_Navicella * sizeof(Point), &Navicella[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);
	
	//Disegna tentacoli
	disegna_tentacoli(col_rosso_scuro, col_verde_scuro, Tentacolo);
	glGenVertexArrays(1, &VAO_TENTACOLO);
	glBindVertexArray(VAO_TENTACOLO);
	glGenBuffers(1, &VBO_N);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_N);
	glBufferData(GL_ARRAY_BUFFER, nVertices_Tentacolo * sizeof(Point), &Tentacolo[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	// definisco le vite e le posizioni iniziali dei tentacoli
	tentacleLives = new int[numTentacles];
	for (int i = 0; i < numTentacles; i++)
	{
		tentacleLives[i] = tentacleLife;
		posxT[i] = i * (passo_Nemici)+passo_Nemici / 2;
		posyT[i] = height+gapTentacoli*i-100;
		frame_animazione_tentacoli_y[i] = gapTentacoli * i - 100;
		frame_animazione_tentacoli_x[i] = width/numTentacles*i;
		float_xoffset_tentacoli[i] = posxT[i];
		printf("%s", tentacleLives[i] ? "true \n" : "false \n");
	}

	//Disegna boss
	disegna_boss(col_nero, col_verde_scuro, col_nero, col_rosso_scuro, Boss);
	glGenVertexArrays(1, &VAO_BOSS);
	glBindVertexArray(VAO_BOSS);
	glGenBuffers(1, &VBO_N);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_N);
	glBufferData(GL_ARRAY_BUFFER, nVertices_Boss * sizeof(Point), &Boss[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	
	//Definisco il colore che verrà assegnato allo schermo
	glClearColor(1.0, 0.5, 0.0, 1.0);

	Projection = ortho(0.0f, float(width), 0.0f, float(height));
	MatProj = glGetUniformLocation(programId, "Projection");
	MatModel = glGetUniformLocation(programId, "Model");
}


void resize(int w, int h)
{
	if (h == 0)	// Window is minimized
		return;
	int width_ = h * aspect_ratio;           // width is adjusted for aspect ratio
	int left = (w - width_) / 2;
	// Set Viewport to window dimensions
	glViewport(left, 0, width_, h);
	width = w;
	height = h;

	// Fixed Pipeline matrices for retro compatibility
	glUseProgram(0); // Embedded openGL shader
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glUseProgram(programId);
}

//scrive VITTORIA o SCONFITTA alla fine della partita
void endGame(int value){

	if (gameStatus!=0) {

		//glUseProgram(0);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, height*aspect_ratio, 0, height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		vector<string> result;

		if (gameStatus == 1) {

			//glColor3f(0.0f, 1.0f, 0.0f); // Imposta il colore del testo
			//glRasterPos2f(-0.2f, 0.0f); // Imposta la posizione della scritta
			if (spaceshipLife == spaceship_life) {
				//glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)"WOW, VITTORIA PERFETTA!!!");
				result.push_back("WOW, VITTORIA PERFETTA!!!");
				printf("STAMPO VITORIA PERFETTA\n");
			}else {
				//glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)"VITTORIA!");
				result.push_back("VITTORIA!");
				printf("STAMPO VITORIA\n");

			}
		}else if (gameStatus == 2) {
			//glColor3f(1.0f, 0.0f, 0.0f); // Imposta il colore del testo
			//glRasterPos2f(-0.2f, 0.0f); // Imposta la posizione della scritta
			//glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)"....sconfitta....");
			result.push_back("...SCONFITTA...");
			printf("STAMPO SCONFITTA\n");

		}

		glDisable(GL_DEPTH_TEST);
		HUD_Logger::get()->printInfo(result);
		glEnable(GL_DEPTH_TEST);

		resize(width, height);
		//glUseProgram(programId);

		//glutSwapBuffers();
	}else {
		//glUseProgram(0);
		glutTimerFunc(500, endGame, 0);
	}
}

//scrive la vita di boss e navicella a schermo
void printLives() {
	
	//glUseProgram(0);
	string lives = "BOSS: " + to_string(bossLife) + "/" + to_string(boss_life) + "\nSpaceship: " + to_string(spaceshipLife) + "/" + to_string(spaceship_life);
	vector<string> lines;
	lines.push_back(lives);
	glDisable(GL_DEPTH_TEST);
	HUD_Logger::get()->printInfo(lines);
	glEnable(GL_DEPTH_TEST);

	resize(width, height);
	/*glColor3f(1.0, 1.0, 1.0);
	glRasterPos2f(width/2, height/2);
	for (char c : lives) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
	}*/

	/*int len, i;
	char lives[] = "BOSS";
	glRasterPos2f(width / 2, height / 2);
	len = (int)strlen(lives);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, lives[i]);
	}*/
	//glUseProgram(programId);
	//glutTimerFunc(100, printLives, 0);
	
}

void drawScene(void)
{
	glUniformMatrix4fv(MatProj, 1, GL_FALSE, value_ptr(Projection));
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(programId);  

	// Disegna spazio
	glBindVertexArray(VAO_SPACE);
	Model = mat4(1.0);
	glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
	glDrawArrays(GL_TRIANGLES, 0, vertices_space);
	glBindVertexArray(0);
	
	//Disegno Navicella e Proiettile
	glBindVertexArray(VAO);
	if (spaceshipLife > 0) {
		Model = mat4(1.0);
		Model = translate(Model, vec3(posx, posy, 0.0));
		Model = scale(Model, vec3(80.0, 40.0, 1.0));
		Model = rotate(Model, radians(angolo), vec3(0.0, 0.0, 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		glDrawArrays(GL_TRIANGLES, 0, nVertices_Navicella - 1);
		//Disegno il proiettile
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glPointSize(8.0);
		Model = mat4(1.0);
		Model = translate(Model, vec3(posx + posx_Proiettile, posy + posy_Proiettile, 0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		glDrawArrays(GL_POINTS, nVertices_Navicella - 1, 1);
	}
	glBindVertexArray(0);
	
	// Disegna tentacoli
	glBindVertexArray(VAO_TENTACOLO);
	for (int i = 0; i < numTentacles; i++)
	{
		
		if (tentacleLives[i]>0 && bossLife>0) {
			Model = mat4(1.0);
			Model = translate(Model, vec3(posxT[i], posyT[i], 0));
			Model = scale(Model, vec3(30.0, 30.0, 1.0));
			Model = rotate(Model, radians(angolo)/30, vec3(0.0, 0.0, 1.0));
			glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// corpo tentacolo
			glDrawArrays(GL_TRIANGLES, 0, nVertices_Tentacolo);
		}
	}
	glBindVertexArray(0);


	// Disegna boss
	glBindVertexArray(VAO_BOSS);
	if (bossLife > 0) {
		Model = mat4(1.0);
		Model = translate(Model, vec3(posxB, posyB, 0));
		Model = scale(Model, vec3(30.0, 30.0, 1.0));
		Model = rotate(Model, radians(angolo) / 10, vec3(0.0, 0.0, 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		// faccia e occhi
		glDrawArrays(GL_TRIANGLES, 0, nVertices_Boss - nvBocca);
		glLineWidth(8.0);  // bocca
		glDrawArrays(GL_LINE_STRIP, nVertices_Boss - nvBocca, nvBocca);
		glLineWidth(8.0);
	}

	glBindVertexArray(0);

	// calcolo collisione proiettile-tentacolo
	for (int i = 0; i < numTentacles; i++)
	{
		//	printf("Posizione del proiettile: x= %f y=%f \n", posx + posx_Proiettile, posy + posy_Proiettile);
		//	printf("Bounding Box (BB) nemico %d %d : xmin= %f ymin=%f  xmax=%f ymax=%f \n", i,j, posxN - 50 , posyN-50, + posx_Proiettile, posy + posy_Proiettile);
		if (((posx + posx_Proiettile >= posxT[i] - 5) && (posx + posx_Proiettile <= posxT[i] + 5)) && ((posy + posy_Proiettile >= posyT[i] - 320) && (posy + posy_Proiettile <= posyT[i])) && gameStatus == 0)
		{
			if (tentacleLives[i]>0)
			{
				tentacleLives[i]--;
				printf("Vite tentacoli: { ");
				for (int j = 0;j < numTentacles;j++) {
					printf("%d ", tentacleLives[j]);
				}
				printf("}\n");
			}
			
		}
	}

	// calcolo collisione proiettile-boss
	//	printf("Posizione del proiettile: x= %f y=%f \n", posx + posx_Proiettile, posy + posy_Proiettile);
	//	printf("Bounding Box (BB) nemico %d %d : xmin= %f ymin=%f  xmax=%f ymax=%f \n", i,j, posxN - 50 , posyN-50, + posx_Proiettile, posy + posy_Proiettile);
	if (((posx + posx_Proiettile >= posxB - 110) && (posx + posx_Proiettile <= posxB + 110)) && ((posy + posy_Proiettile >= posyB - 20) && (posy + posy_Proiettile <= posyB + 15)) && gameStatus == 0)
	{
		if (bossLife > 0) {
			bossLife--;
			printf("Vita boss: { %d }\n", bossLife);
			if (bossLife > boss_life * 0.33 && bossLife < boss_life * 0.66) {
				bossPhase = 1;
			}
			else if (bossLife < boss_life * 0.33) {
				bossPhase = 2;
			}
		}
		else {
			gameStatus = 2;
			printf("VITTORIA\n");
		}
	}

		// calcolo collisione tentacolo - navicella
	for (int i = 0; i < numTentacles; i++) {
		//	printf("Posizione del proiettile: x= %f y=%f \n", posx + posx_Proiettile, posy + posy_Proiettile);
		//	printf("Bounding Box (BB) nemico %d %d : xmin= %f ymin=%f  xmax=%f ymax=%f \n", i,j, posxN - 50 , posyN-50, + posx_Proiettile, posy + posy_Proiettile);
		if (((posx + 20 >= posxT[i] - 5) && (posx + 20 <= posxT[i] + 5)) && ((posy + 20 >= posyT[i] - height) && (posy + 20 <= posyT[i])) && gameStatus==0)
		{
			if (spaceshipLife > 0) {
				spaceshipLife--;
				printf("Vita navicella: { %d }\n", spaceshipLife);
			}else {
				gameStatus = 1;
				printf("SCONFITTA\n");
			}
		}
	}
	/*
	if (gameStatus != 0) {
		glUseProgram(0);
		endGame(0);
		glUseProgram(programId);
	}
	*/

	//glUseProgram(0);
	//printLives();

	glutSwapBuffers();
}

void refresh_monitor(int millis)
{
	glutPostRedisplay();
	glutTimerFunc(millis, refresh_monitor, millis);
}

int main(int argc, char* argv[])
{

	glutInit(&argc, argv);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("Cosmic Enemy");

	glutDisplayFunc(drawScene);
	glutReshapeFunc(resize);
	//Evento tastiera tasto premuto
	glutKeyboardFunc(keyboardPressedEvent);
	//Evento tastiera tasto rilasciato
	glutKeyboardUpFunc(keyboardReleasedEvent);
	// glutTimerFunc a timer callback is triggered in a specified number of milliseconds
	glutTimerFunc(500, updateP, 0);
	glutTimerFunc(500, updateV, 0);
	glutTimerFunc(500, updateTentacoli, 0);
	glutTimerFunc(500, updateBoss, 0);
	//glutTimerFunc(500, endGame, 0);

	//glutTimerFunc(500, printLives, 0);
	//printLives(0);
	glewExperimental = GL_TRUE;
	glewInit();
	//refresh_monitor(16);
	initShader();
	init();

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glutMainLoop();
}