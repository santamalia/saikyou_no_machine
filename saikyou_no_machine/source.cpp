#include "main.h"

static double px = QX, py = QY, pz = QZ;   /* ボールの初期座標 */
static double bx = -0.7, by = 1.35, bz = 18.0; /* バットの初期座標 */
static double mx = 0.0, my = 1.35, mz = 18.0; /* ミートカーソルの中心座標*/
static double cam_x = 0.0, cam_y = -QY - 0.7, cam_z = -26.4; /* カメラのx,y座標 */
double v = 0;                             /* 速度 */
double theta = 0, phi = 0;
double vx = v * sin(theta) * cos(phi), vy = v * sin(phi), vz = v * cos(theta) * cos(phi);
double az = 5.0; /* Z軸方向の加速度 */
double vz_achieve;
double t_achieve; // 着弾する時間
double px_achieve, py_achieve;
double fd; //打球の飛距離:frying distance
unsigned int tn = 0, tn_hitted;
double t = 0; /* 実行してからの経過時間 */
double click_time = 200;
double click_x, click_y, MouseX2, MouseY2;
double dt = TSTEP;
int s_count, b_count, out, four;
char judge[100];
int pitched = 0, swinged = 0, hitted = 0, batted = 0; /* 投球、スイング、バットとの衝突、打球が飛んでいる、が起きたら1、それ以外は0 */
double swinged_time; /* スイングされた時刻 */
double bat_angle = 50, bat_angle_hitted = 0, arm_angle = 0, cam_x_angle = 0, cam_y_angle = 0; /* バット、腕、カメラの回転角 */
double r, th; // 打球の角度
int species = 0; // 球種 0:ストレート 1:スライダー 2:カーブ 3:フォーク 4:シンカー 5:シュート 6:ライジングキャノン 7:Wボール

GLfloat white[] = { 1.f, 1.f, 1.f, 1.f }; /* 白色 */
GLfloat deep_green[] = { 0.1f, 0.7f, 0.1f, 1.f}; /* 甲子園グリーン */
GLfloat brown[] = { 0.7f, 0.5f, 0.f, 0.5f }; /* 土の色 */
GLuint textureID;
GLuint loadTexture(char* filename);
GLuint textureID_score[3];

double coord[2] = { 0.0, 0.0 };

void displaySphere(float radius, float dr, float dg, float db, float sr, float sg, float sb, float shininess)
{

	GLfloat specular[4] = { sr, sg, sb, 1.f };

	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
	glutSolidSphere(radius, 100, 100);
}

/*
* 大地の描画
*/
static void myGround(double height)
{
	const static GLfloat ground[][4] = {
		{ 1.0, 1.0, 1.0, 1.0 },
		{ 0.8, 0.8, 0.8, 1.0 }
	};

	

	int i, j;

	glBindTexture(GL_TEXTURE_2D, textureID);
	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glNormal3d(0.0, 1.0, 0.0);
	for (j = -D / 2; j < D / 2; ++j) {
		for (i = -W / 2; i < W / 2; ++i) {
			glMaterialfv(GL_FRONT, GL_DIFFUSE, ground[(i + j) & 1]);
			glTexCoord2d(0.0, 0.0);
			glVertex3d((GLdouble)i, height, (GLdouble)j);
			glTexCoord2d(0.0, 1.0);
			glVertex3d((GLdouble)i, height, (GLdouble)(j + 1));
			glTexCoord2d(1.0, 1.0);
			glVertex3d((GLdouble)(i + 1), height, (GLdouble)(j + 1));
			glTexCoord2d(1.0, 0.0);
			glVertex3d((GLdouble)(i + 1), height, (GLdouble)j);
		}
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);

	/* マウンドの描画 */
	glPushMatrix();
	glTranslated(0.0, -4.5, 0.0);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, brown);
	glutSolidSphere(5.0, 100, 100);
	glPopMatrix();
}

/* スコアボードの描画 */
static void scoreboard(int n, double z, double w, double h, double y){
	glBindTexture(GL_TEXTURE_2D, textureID_score[n]);
	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glNormal3d(0.0, 0.0, 1.0);

	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glTexCoord2d(0.0, 0.0);
	glVertex3d(-w / 2, y, z);
	
	glTexCoord2d(1.0, 0.0);
	glVertex3d(w / 2, y, z);

	glTexCoord2d(1.0, 1.0);
	glVertex3d(w / 2, h + y, z);
	
	glTexCoord2d(0.0, 1.0);
	glVertex3d(-w / 2, h + y, z);
	
	
	

	glEnd();
	glDisable(GL_TEXTURE_2D);
}

static void DrawString(char str[], int w, int h, int x0, int y0)
{
	//glDisable(GL_LIGHTING);
	// 平行投影にする
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// 画面上にテキスト描画
	glRasterPos2f(x0, y0);
	int size = 0;
	while (str[size] != 0x00){
		size++;
	}

	for (int i = 0; i < size; ++i){
		char ic = str[i];
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, ic);
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

/* ストライクゾーンの描画 */
void zone() {
	double vertices[][3] = { /* ゾーンの座標 */
		{ -0.3, 1.0, 18.0 },
		{ 0.3, 1.0, 18.0 },
		{ 0.3, 1.7, 18.0 },
		{ -0.3, 1.7, 18.0 }
	};

	GLfloat zone_color[] = { 1, 0.1, 0.4, 1 }; /* ゾーンの色 */

	glNormal3f(0.0, 0.0, 1.0);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, zone_color);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 4; i++) {
		glVertex3dv(vertices[i]);
	}
	glEnd();
}

/* ホームベースの描画 */
void base(){
	double base_v[][3] = {
		{ 0.0, 0.6, 18.8 },
		{ -0.3, 0.6, 18.4 },
		{ -0.3, 0.6, 18.0 },
		{ 0.3, 0.6, 18.0 },
		{ 0.3, 0.6, 18.4 }
	};

	glNormal3d(0.0, 1.0, 18.0);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 5; i++) {
		glVertex3dv(base_v[i]);
	}
	glEnd();
}

/* 塁線の描画 */
void line(){
	double edge[][3] = {
			{ -0.3, 0.6, 18.4 },
			{ -17.9, 0.6, 0.0 },
			{ 0.3, 0.6, 18.4 },
			{ 17.9, 0.6, 0.0 }
	};
	glLineWidth(3.0);

	glNormal3d(0.0, 1.0, 18.0);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glBegin(GL_LINES);
	glVertex3dv(edge[0]);
	glVertex3dv(edge[1]);
	glEnd();

	glNormal3d(0.0, 1.0, 18.0);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glBegin(GL_LINES);
	glVertex3dv(edge[2]);
	glVertex3dv(edge[3]);
	glEnd();

	glLineWidth(1.0);
}

/* バッターボックスの描画 */
/* スコアボードの描画 */

/* 直方体を描画する関数(横幅w, 高さh, 深さd, X座標x, Y座標y, Z座標z) */
static void myBox(double w, double h, double d, double x, double y, double z){
  GLdouble vertex[][3] = {
    { -w + x, y, -d + z },
    {  w + x, y, -d + z },
    {  w + x,  h + y, -d + z },
    { -w + x,  h + y, -d + z },
    { -w + x, y, d + z },
    {  w + x, y, d + z },
    {  w + x,  h + y, d + z },
    { -w + x,  h + y, d + z }
  };

  const static int face[][4] = {
    { 0, 1, 2, 3 },
    { 1, 5, 6, 2 },
    { 5, 4, 7, 6 },
    { 4, 0, 3, 7 },
    { 4, 5, 1, 0 },
    { 3, 2, 6, 7 }
  };

  const static GLdouble normal[][3] = {
    { 0.0, 0.0,-1.0 },
    { 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0 },
    {-1.0, 0.0, 0.0 },
    { 0.0,-1.0, 0.0 },
    { 0.0, 1.0, 0.0 }
  };

  const static GLfloat red[] = { 0.8, 0.2, 0.2, 1.0 };

  int i, j;

  /* 材質を設定する */
  glMaterialfv(GL_FRONT, GL_DIFFUSE, red);

  glBegin(GL_QUADS);
  for (j = 0; j < 6; ++j) {
    glNormal3dv(normal[j]);
    for (i = 4; --i >= 0;) {
      glVertex3dv(vertex[face[j][i]]);
    }
  }
  glEnd();
}


/* X-Y平面に、円を描画する関数 */
void PrintCircle(double r, int n, double x, double y, double z){
	glNormal3f(0.0, 0.0, 1.0);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < n; i++){
		glVertex3f(x + r * cos(2.0 * 3.14926535 * ((double)i / n)), y + r * sin(2.0 * 3.14926535 * ((double)i / n)), z);
	}
	glEnd();
}

/* X-Y平面に、ミートカーソルを描画する関数 */
void PrintEllipse(double r, int n, double x, double y, double z){
	PrintCircle(0.01, 10, x, y, z);
	glNormal3f(0.0, 0.0, 1.0);
	glBegin(GL_LINE_LOOP);
	glVertex3d(x + r, y, z);
	glVertex3d(x + 0.02, y, z);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3d(x - r, y, z);
	glVertex3d(x - 0.02, y, z);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3d(x, y + (r - 0.07), z);
	glVertex3d(x, y + 0.02, z);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3d(x, y - 0.02, z);
	glVertex3d(x, y - (r - 0.07), z);
	glEnd();
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < n; i++){
		glVertex3f(x + r * cos(2.0 * 3.14926535 * ((double)i / n)), y + (r - 0.07) * sin(2.0 * 3.14926535 * ((double)i / n)), z);
	}
	glEnd();
}

void myCylinder(double radius, double height, int sides)
{
	double step = 2 * PI / (double)sides;
	int i;
	GLfloat cylinder_color[] = { 1.f, 1.f, 1.f, 1.f }; /* 円柱の色 */

	/* 上面 */
	glNormal3d(0.0, 1.0, 0.0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, cylinder_color);
	glBegin(GL_POLYGON);
	glBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < sides; i++) {
		double t = step * (double)i;
		glVertex3d(radius * sin(t), height, radius * cos(t));
	}
	glEnd();

	/* 底面 */
	glNormal3d(0.0, -1.0, 0.0);
	glBegin(GL_TRIANGLE_FAN);
	for (i = sides; --i >= 0;) {
		double t = step * (double)i;
		glVertex3d(radius * sin(t), 0.0, radius * cos(t));
	}
	glEnd();

	/* 側面 */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= sides; i++) {
		double t = step * (double)i;
		double x = sin(t);
		double z = cos(t);

		glNormal3d(x, 0.0, z);
		glVertex3f(radius * x, height, radius * z);
		glVertex3f(radius * x, 0.0, radius * z);
	}
	glEnd();
}

/* 長方形を表示 引数:x,y,z座標,高さ,幅,x-z平面となす角 */
void myPalse(double x, double y, double z, double h, double w, int angle){
	double v[][3] = {
		{ x - w / 2, y + h / 2 * sin(angle / 180 * PI), z + h / 2 * cos(angle / 180 * PI) },
		{ x - w / 2, y - h / 2 * sin(angle / 180 * PI), z - h / 2 * cos(angle / 180 * PI) },
		{ x + w / 2, y - h / 2 * sin(angle / 180 * PI), z - h / 2 * cos(angle / 180 * PI) },
		{ x + w / 2, y - h / 2 * sin(angle / 180 * PI), z + h / 2 * cos(angle / 180 * PI) }
	};
	glMaterialfv(GL_FRONT, GL_DIFFUSE, deep_green);
	glNormal3d(0.0, sin((90 - angle) / 180 * PI), cos((90 - angle) / 180 * PI));
	glBegin(GL_QUADS);
	for (int i = 0; i < 4; i++){
		glVertex3dv(v[i]);
	}
	glEnd();
}

/* 座標(x, z)に投球アニメーションを表示 */
void pitcher(double x, double y, double z){
	/* 色 */
	GLfloat white[] = { 1.f, 1.f, 1.f, 1.f };
	GLfloat red[] = { 1.f, 0.0f, 0.0f, 1.f };
	GLfloat beige[] = { 1.f, 0.9f, 0.7f, 1.f };
	GLfloat black[] = { 0.f, 0.f, 0.f, 0.f };

	/* 腕と手 */
	glPushMatrix();
	glTranslated(x, y + 0.6, z);
	glRotated(arm_angle, 1.0, 0.0, 0.0);
	glTranslated(0, -0.2, 0);
	glEnable(GL_NORMALIZE);
	glScalef(0.3f, 1.f, 0.3f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, red);
	displaySphere(0.2f, 0.9f, 0.6f, 0.6f, 0.2f, 0.2f, 0.2f, 10.f);/* 腕 */
	glTranslated(0, -0.2, 0);
	glScalef(1.f, 0.3f, 1.f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, beige);
	displaySphere(0.3f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f); /* 手 */
	glDisable(GL_NORMALIZE);
	glPopMatrix();

	glPushMatrix();
	glTranslated(-x - 0.05, y + 0.6, z);
	glRotated(0.0, 1.0, 0.0, 0.0);
	glTranslated(0, -0.2, 0);
	glEnable(GL_NORMALIZE);
	glScalef(0.3f, 1.f, 0.3f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, red);
	displaySphere(0.2f, 0.9f, 0.6f, 0.6f, 0.2f, 0.2f, 0.2f, 10.f);/* 腕 */
	glTranslated(0, -0.2, 0);
	glScalef(1.f, 0.3f, 1.f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, beige);
	displaySphere(0.3f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f); /* 手 */
	glDisable(GL_NORMALIZE);
	glPopMatrix();

	/* 胴体 */
	glPushMatrix();
	glTranslated(x + 0.16, y, z - 0.1);
	myCylinder(0.15, 0.6, 1000);
	glPopMatrix();

	/* 頭 */
	glPushMatrix();
	glTranslated(0, y + 0.8, z - 0.1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, beige);
	displaySphere(0.2f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f);/* 顔面 */
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, y + 0.88, z - 0.1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	//glScalef(1.f, 0.3f, 1.f); //はちまきっぽいなんか
	glScalef(1.f, 0.7f, 1.f);
	displaySphere(0.21f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f);/* 帽子 */
	glPopMatrix();
	
	glPushMatrix();
	glTranslated(0, y + 0.8, z - 0.2);
	glScalef(0.3f, 0.07f, 1.f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black);
	glNormal3d(0.0, 0.0, 1.0);
	displaySphere(0.8f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f);/* 帽子のつば */
	glPopMatrix();
}

void pitching(int n){ // 投球開始前初期化処理 引数：球種
	pitched = 1;
	swinged = 0;
	batted = 0;
	bat_angle = 50;
	species = n;
	tn = 0;

	px = QX;
	py = QY;
	pz = QZ;

	mx = 0;
	my = 1.35;
	double vo[] = { 40.0, 40.0, 40.0, 40.0, 40.0, 40.0 ,50.0 ,45.0 };
	v = rand() % 20 + vo[n];
	theta = (double)((rand() % 45) * 0.05 +  0.0) / 180 * PI; /* 横の角度 */
	phi = (double)((rand() % 40) * 0.2 - 1.5) / 180 * PI;	/* 縦の角度*/

	/*v = 50;
	theta = 0.01;
	phi = 0.01*/

	vx = v * sin(theta) * cos(phi);
	vy = v * sin(phi);
	vz = v * cos(theta) * cos(phi);
	if (n == 0 || n == 6 || n == 7){
		az = 10.0;
	}
	else{
		az = 1.0;
	}
}

void batter(double x, double y, double z){
	/* 色 */
	GLfloat white[] = { 1.f, 1.f, 1.f, 1.f };
	GLfloat blue[] = { 0.f, 0.0f, 1.0f, 1.f };
	GLfloat beige[] = { 1.f, 0.9f, 0.7f, 1.f };
	GLfloat black[] = { 0.f, 0.f, 0.f, 0.f };

	/* 腕と手 */

	glPushMatrix();
	glTranslated(x + 0.1, y + 0.58, z);
	glRotated(-90, 1.0, 0.0, 0.0);
	glRotated(bat_angle - 70, 0.0, 0.0, 1.0);
	glTranslated(-0.1, -0.24, 0);
	glEnable(GL_NORMALIZE);
	glScalef(0.3f, 1.f, 0.3f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, blue);
	displaySphere(0.2f, 0.9f, 0.6f, 0.6f, 0.2f, 0.2f, 0.2f, 10.f);/* 腕 */
	glTranslated(0, -0.2, 0);
	glScalef(1.f, 0.3f, 1.f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, beige);
	displaySphere(0.3f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f); /* 手 */
	glDisable(GL_NORMALIZE);
	glPopMatrix();

	glPushMatrix();
	glTranslated(x + 0.3, y + 0.58, z);
	glRotated(-90, 1.0, 0.0, 0.0);
	glRotated(bat_angle - 110, 0.0, 0.0, 1.0);
	glTranslated(0.2, -0.4, 0);
	//glTranslated(0, -0.2, 0);
	glEnable(GL_NORMALIZE);
	glScalef(0.3f, 1.f, 0.3f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, blue);
	displaySphere(0.2f, 0.9f, 0.6f, 0.6f, 0.2f, 0.2f, 0.2f, 10.f);/* 腕 */
	glTranslated(0, -0.2, 0);
	glScalef(1.f, 0.3f, 1.f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, beige);
	displaySphere(0.3f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f); /* 手 */
	glDisable(GL_NORMALIZE);
	glPopMatrix();

	/* 胴体 */
	glPushMatrix();
	glTranslated(x + 0.16, y, z - 0.1);
	myCylinder(0.15, 0.6, 1000);
	glPopMatrix();

	/* 頭 */
	glPushMatrix();
	glTranslated(x+0.18, y + 0.8, z - 0.1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, beige);
	displaySphere(0.2f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f);/* 顔面 */
	glPopMatrix();

	glPushMatrix();
	glTranslated(x+0.18, y + 0.88, z - 0.1);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	//glScalef(1.f, 0.3f, 1.f); //はちまきっぽいなんか
	glScalef(1.f, 0.7f, 1.f);
	displaySphere(0.21f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f);/* 帽子 */
	glPopMatrix();

	glPushMatrix();
	glTranslated(x+0.18, y + 0.8, z - 0.2);
	glScalef(0.3f, 0.07f, 1.f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black);
	glNormal3d(0.0, 0.0, 1.0);
	displaySphere(0.8f, 0.6f, 0.2f, 0.1f, 1.f, 1.f, 1.f, 100.f);/* 帽子のつば */
	glPopMatrix();
}

void judge_s_or_b(){
	/* ストライク・ボール判定 */
	if (fabs(px_achieve) < 0.3 && py_achieve > 1.0 && py_achieve < 1.7) {
		s_count++;
	}
	else if (swinged == 1 && hitted == 0){
		s_count++;
	}
	else {
		b_count++;
	}

	/* 三振・四球判定 */
	if (s_count == 3){
		out = 1;
		s_count = 0;
		b_count = 0;
	}
	else if (b_count == 4){
		four = 1;
		s_count = 0;
		b_count = 0;
	}
	else{
		out = 0;
		four = 0;
	}

	//printf("achieve = (%f, %f, 18.0)\n", px_achieve, py_achieve);
	//printf("v = (%f, %f, %f) theta = %f phi = %f\n", vx, vy, vz, theta, phi);
	//printf("t = %f\n", t_achieve);
}

void idle(void) {
	glutPostRedisplay();
}

/*
* ディスプレイ関数
*/
static void display(void)
{
	const static GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };    /* ���̐F */
	const static GLfloat lightpos[] = { 12.0, 16.0, 20.0, 1.0 }; /* �����̈ʒu */

	/* ���ʃN���A */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	t = dt * tn;

	/* ���f���r���[�ϊ��s���̏����� */
	glLoadIdentity();

	if (batted) gluLookAt(px, py, pz, px, py - 2, pz - 15, 0, 0.8, 0.4);
	else gluLookAt(0, 0, 0, cam_x, cam_y, cam_z, 0, 1, 0);
	
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	/* 回転 */
	glRotated(cam_x_angle, 1.0, 0.0, 0.0);
	glRotated(cam_y_angle, 0.0, 1.0, 0.0);

		

	/* �����̈ʒu���ݒ� */
	
	/* カメラの位置 */
	glTranslated(cam_x, cam_y, cam_z);

	zone(); // ストライクゾーンの描画	
	myGround(0.0); // 大地の描画

	base(); //ホームベースの描画
	line(); //塁線の描画
	scoreboard(0, -60, 15.0, 3.5, 1.25);
	//scoreboard(1, -60, 1.7, 1.5, 3.0);
	//scoreboard(2, -60, 1.1, 0.56, 3.75);

	//myBox(15, 13, 5, 0, 1, -80);
	//glPushMatrix();
	//glRotated(10, 1.0, 0.0, 0.0);
	//myPalse(0.0, 5.0, -10.0, 10.0, 8.0, 90);
	//glPopMatrix();

	tn++;

	/* ピッチャーの描画 */
	pitcher(QX, QY - 0.9, QZ);

	if (batted){ // 打球の軌道計算
		px = mx + vx * (t - 2.0);
		py = my + 0.5 * G * (t - 2.0) * (t - 2.0) + vy * (t - 2.0);
		pz = 16.0 + 6.5 + 0.5 * az * (t - 2.0) * (t - 2.0) + vz * (t - 2.0);

		/*px = px_achieve + vx * t;
		py = py_achieve + 0.5 * G * t * t + vy * t;
		pz = 18.0 + 0.5 * az * t * t + vz * t;*/
	} else { // 投球の軌道計算
		px = vx * (t - 2.0) + QX;
		py = 0.5 * G * (t - 2.0) * (t - 2.0) + vy * (t - 2.0) + QY;
		pz = 0.5 * az * (t - 2.0) * (t - 2.0) + vz * (t - 2.0) + QZ;

	}

	/* 着弾点の x, y座標を計算 */
	if (pz < 18.0){
		t_achieve = (-vz + sqrt(vz * vz + 2 * az * 18.0)) / az;
		px_achieve = vx * t_achieve + QX;
		py_achieve = 0.5 * G * t_achieve * t_achieve + vy * t_achieve + QY;
	}

	//printf("t = %f\n", t);
	//printf("(px, py, pz) = (%f, %f, %f)\n", px, py, pz);
	//printf("(vx, vy, vz) = (%f, %f, %f)\n", vx, vy, vz);
	/* 床との衝突 */
	if (py < R){
		py = R;
		vy *= -A;
	}
	/* スライダー */
	if (species == 1 && pz > 12 && pz < 18) {
		vx += 0.002 * pow(t, 4);
	}
	/* カーブ */
	if (species == 2 && pz > 12 && pz < 18) {
		vx += 0.001 * pow(t, 4);
		vy -= 0.002 * pow(t, 4);
	}
	/* スプリット */
	if (species == 3 && pz > 12 && pz < 18) {
		vy -= 0.002 * pow(t, 4);
	}
	/* シンカー */
	if (species == 4 && pz > 12 && pz < 18) {
		vx -= 0.001 * pow(t, 4);
		vy -= 0.002 * pow(t, 4);
	}
	/* シュート */
	if (species == 5 && pz > 12 && pz < 18) {
		vx -= 0.002 * pow(t, 4);
	}
	/* ライジングキャノン */
	if (species == 6 && pz > 12 && pz < 18){
		vy += 0.0015 * pow(t, 4);
	}
	/* Wボール */
	if (species == 7 && pz > 6 && pz < 9){
		vy -= 0.04 * pow(t, 4);
	}
	else if (species == 7 && pz > 9 && pz < 12){
		vy += 0.09 * pow(t, 4);
	}
	else if (species == 7 && pz > 12 && pz < 15){
		vy -= 0.07 * pow(t, 4);
	}
	else if (species == 7 && pz > 15 && pz < 18){
		vy += 0.04 * pow(t, 4);
	}

	char species_name[][15] = { "STRAIGHT", "SLIDER", "CURVE", "SFF", "SINKER", "SHOOT", "RISING_CANON", "W_BALL" };
	/* 球速の表示 */
	char str[100], vz_s[4] = { '\0' }, pz_s[4];
	if (t > t_achieve - 0.01 && t < t_achieve + 0.01){
		vz_achieve = vz;
	}
	if (t > t_achieve + 1.8){
		sprintf_s(vz_s, "%3d", (int)(vz_achieve * 2.7));
		strcpy_s(str, "Speed: ");
		strcat_s(str, vz_s);
		strcat_s(str, "km/h");
		//strcat_s(str, species[key]);
		if ((fabs(px_achieve) < 0.3 && py_achieve > 1.0 && py_achieve < 1.7) || (swinged == 1 && hitted == 0)) {
			strcpy_s(judge, " Judge: STRIKE! Count: S| ");
		}
		else if (vz < 0){
			strcpy_s(judge, " Judge: HIT! Count: S| ");
		}
		else if (!(fabs(px_achieve) < 0.3 && py_achieve > 1.0 && py_achieve < 1.7)) {
			strcpy_s(judge, " Judge: BALL. Count: S| ");
		}
		for (int i = 0; i < s_count; i++)strcat_s(judge, "*");
		strcat_s(judge, " B| ");
		for (int i = 0; i < b_count; i++)strcat_s(judge, "*");
		if (out == 1)strcat_s(judge, " BATTER OUT!");
		else if (four == 1)strcat_s(judge, " FOUR BALL!");
		//else if (az < 0) strcat_s(judge, " HIT!");
		strcat_s(str, judge);
		strcat_s(str, " Species: ");
		strcat_s(str, species_name[species]);
		if (batted == 1){
			fd = hypot(px, -pz + 18.0);
			sprintf_s(pz_s, "%3d", (int)(fd));
			strcat_s(str, " Frying Distance: ");
			strcat_s(str, pz_s);
			strcat_s(str, "m");
		}
		DrawString(str, 500, 200, 100, 10);
	}

	/* 着弾点を表示 */
	if (pitched){
		GLfloat white[] = { 1.f, 1.f, 1.f };
		if (t > 1.0 && t < 2.0){
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
			PrintCircle(2.0 - 1.0 * t + R, 100, px_achieve, py_achieve, 18.0); //円を描画 (半径, 分割数, X座標, Y座標, Z座標)
		}
		else if (t > 2.0){
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
			PrintCircle(R, 100, px_achieve, py_achieve, 18.0);
		}
		if (t < 2.0){
			arm_angle = 50 * t * t;
		}
		else if (t > 1.8 && t < 2.17){
			arm_angle += 2.0 * t * t;
		}
	}

	if (swinged){
		bat_angle += VS;
	}
	if (bat_angle > 300){
		bat_angle = 300;
	}
	/* 衝突時の処理 */
	if (hitted){
		bat_angle_hitted = bat_angle + 400 * (2 * t_achieve - 2 * swinged_time + 2.15); // ボールと衝突時のバットの角度を計算
		//th = 10 * PI / 180;
		th = (2 * (bat_angle_hitted - 180) - phi) * PI / 180; /* 打球のY-Z平面となす角度の計算 */
		//r = (200 * (py_achieve - my + 0.1) / 0.125) * PI / 180; /* 打球とX-Z平面となす角度の計算 */
		r = 195 * PI / 180;

		v = -0.3500 / (fabs(mx - px_achieve) + fabs(my - py_achieve)) * 0.5 * 4;

		px = px_achieve;
		py = py_achieve;
		pz = 18.0;
		az = -0.1;

		vx = v * sin(th) * cos(r);
		vy = v * sin(r);
		vz = v * cos(th) * cos(r);

		hitted = 0;
		printf("th = %f\n dt = %f\n",th,t_achieve - swinged_time + 2.0);
	}

	if (t > 2.0 && pitched){
		glPushMatrix();
		glTranslated(px, py, pz);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
		glutSolidSphere(R, 16, 8);
		glPopMatrix();
	}

	/* バッターの描画 */
	batter(mx - 1.2, my - 0.6, 18.0);

	/* バット */
	glPushMatrix();
	glTranslated(mx - 1.2, my, 18.0);
	glRotated(bat_angle, 0.0, 1.0, 0.0);
	glTranslated(-0.8, 0, 0);
	
	glEnable(GL_NORMALIZE);
	GLfloat bat_color[] = { 0.9, 0.7, 0.3, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, bat_color);
	glScalef(1.f, 0.07f, 0.05f);
	displaySphere(0.5f, 0.8f, 0.8f, 0.8f, 0.2f, 0.2f, 0.2f, 10.f);
	glDisable(GL_NORMALIZE);
	glPopMatrix();

	/* ミートカーソルの表示 */

	GLfloat yellow[] = { 1.f, 1.f, 0.f, 1.f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, yellow);
	PrintEllipse(R + 0.1, 100, mx, my, mz);

	glutSwapBuffers();
}

static void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 's': // ストレート
		pitching(0);
		judge_s_or_b();
		break;
	case 'd': // スライダー
		pitching(1);
		judge_s_or_b();
		break;
	case 'c': // カーブ
		pitching(2);
		judge_s_or_b();
		break;
	case 'x': // フォーク
		pitching(3);
		judge_s_or_b();
		break;
	case 'z': // シンカー
		pitching(4);
		judge_s_or_b();
		break;
	case 'a': // シュート
		pitching(5);
		judge_s_or_b();
		break;
	case 'q': // ライジングキャノン
		pitching(6);
		judge_s_or_b();
		break;
	case 'w': // Wボール
		pitching(7);
		judge_s_or_b();
		break;

	case 'U':
		cam_y -= 0.1;
		break;
	case 'D':
		cam_y += 0.1;
		break;
	case 'R':
		cam_x -= 0.1;
		break;
	case 'L':
		cam_x += 0.1;
		break;
	case 'S':
		if (cam_z == 10){
			cam_x = 0.0, cam_y = -QY - 0.7, cam_z = -25;
		} else {
			cam_z = 10;
			cam_x = 1;
		}
	case '\x0D':
		swinged = 1;
		swinged_time = t;
		printf("swinged_time = %f, t_archive = %f fabs() = %f\n",swinged_time, t_achieve, fabs(swinged_time - t_achieve - 2.0));
		printf("dx = %f, dy = %f \n",fabs(mx - px_achieve), fabs(my - py_achieve));
		printf("%f\n",pz);
		/* 衝突判定 */
		if ((fabs(mx - px_achieve) < 0.1250) * (fabs(my - py_achieve) < 0.1000) * (pz > 15.0) * (pz < 19.0)){
			hitted = 1;
			batted = 1;
			s_count = 0;
			b_count = 0;
			printf("hitted!");
			tn_hitted = tn;
		}
		break;
		/* ESC で終了 */
	case '\033':
		exit(0);

	default:
		break;
	}
}

void specialkeydown(int key, int x, int y){
	switch (key){
	case GLUT_KEY_RIGHT:
		mx += DM;
		break;
	case GLUT_KEY_LEFT:
		mx -= DM;
		break;
	case GLUT_KEY_UP:
		my += DM;
		break;
	case GLUT_KEY_DOWN:
		my -= DM;
		break;
	default:
		break;
	}
}

void motionFunction(int x, int y){
	cam_x_angle = -(WINDOW_HEIGHT / 2 - y) * 0.05;
	cam_y_angle = -(x - WINDOW_WIDTH / 2) * 0.05;
	glutPostRedisplay();
}

static void init(void)
{
	/* 光源の設定 */
	glClearColor(0.4, 0.7, 1.0, 1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

int main(int argc, char *argv[])
{
	srand((unsigned)time(NULL));

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("最強のピッチングマシーン");
	glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialkeydown);
	glutMotionFunc(motionFunction);
	init();
	glutIdleFunc(idle);

	textureID = loadTexture("grass.bmp");
	textureID_score[0] = loadTexture("scoreboard0.bmp");
	textureID_score[1] = loadTexture("scoreboard1.bmp");
	textureID_score[2] = loadTexture("scoreboard2.bmp");

	glutMainLoop();
	return 0;
}
