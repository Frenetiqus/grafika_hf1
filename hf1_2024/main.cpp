//=============================================================================================
// Mintaprogram: Z�ld h�romsz�g. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Maller Domonkos
// Neptun : XDK78J
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	layout(location = 0) in vec3 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x/vp.z, vp.y/vp.z, 0, 1);		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

enum class ProgramState { STATE_P, STATE_L, STATE_M, STATE_I };
const char* programStateToString(ProgramState state) {
    switch (state) {
        case ProgramState::STATE_P:
            return "STATE_P";
        case ProgramState::STATE_L:
            return "STATE_L";
        case ProgramState::STATE_M:
            return "STATE_M";
        case ProgramState::STATE_I:
            return "STATE_I";
        default:
            return "Invalid state";
    }
}

// Program variables and helper functions
ProgramState currentState = ProgramState::STATE_P;
const vec3 invalidPoint = vec3(1000000, 1000000, 1000000); // this represents an invalid point in normalized coordinates, as the normalized coordinates go from -1 to 1 then any point outside is invalid
GPUProgram gpuProgram; // vertex and fragment shaders

vec3 clickedPoints[2] = {invalidPoint, invalidPoint};
bool isPointInvalid(vec3 point){
	if (point.x == invalidPoint.x && point.y == invalidPoint.y){
		return true;
	}
	return false;
}
void resetClickedPoints(){
	clickedPoints[0] = invalidPoint;
	clickedPoints[1] = invalidPoint;
}


class Object {
protected:
	unsigned int vao, vbo;
	std::vector<vec3> vertexCoords;

public:
	Object(){
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo); 
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glEnableVertexAttribArray(0); 

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	void addVertex(float x, float y){
		vertexCoords.push_back(vec3(x,y,1.0f));
	}

	void draw(GLenum mode, vec3 color){
		glBindVertexArray(vao);

		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, color.x, color.y, color.z); 

		
		glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER,      // copy to the GPU
			         vertexCoords.size() * sizeof(vec3), // number of the vbo in bytes
					 &vertexCoords[0],		   // address of the data array on the CPU
					 GL_STATIC_DRAW);	
		glDrawArrays(mode, 0, vertexCoords.size());
	}
};

class PointCollection : Object{
public:
	PointCollection() {
	}

	void addPoint(float x, float y){
		printf("Point: %f, %f added\n", x, y);
		addVertex(x, y);
	}

	void drawPoints(){
		draw(GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
	}

	// Return the closest point to given normalized coordinates. If a point exists in 0.01 proximity then it returns, else return invalidPoint which is outside -1, 1
	vec3 getClosestPoint(float cX, float cY){
		vec3 refPoint = vec3(cX, cY, 1);
		for(int i=0; i<vertexCoords.size(); i++){
			if (length(vertexCoords[i]-refPoint) < 0.01){
				return vertexCoords[i];
			}
		}
		return invalidPoint;
	}

};

class Line{
	vec3 p1, p2;
	vec3 norm;
	vec3 dir;


	// returns the max t where the line reaches the boundary based on normalized coordinates (closer than 0.01)
	float tMax(){
		float t = 0.0;
		vec3 r_ = r(t);
		while(r_.x < 1.0 && r_.x >= -1.0 && r_.y < 1.0 && r_.y > -1.0){
			t += 0.001;
			r_ = r(t);
		}
		return t;
	}
	// returns the min t where the line reaches the boundary based on normalized coordinates (closer than 0.01)
	float tMin(){
		float t = 0.0;
		vec3 r_ = r(t);
		while(r_.x < 1.0 && r_.x >= -1.0 && r_.y < 1.0 && r_.y > -1.0){
			t -= 0.001;
			r_ = r(t);
		}
		return t;
	}
public:
	Line(float p1X, float p1Y, float p2X, float p2Y){
		p1 = vec3(p1X, p1Y, 1.0f);
		p2 = vec3(p2X, p2Y, 1.0f);
		norm = cross(p1, p2);
		norm = norm/norm.z;

		printf("Line added\n");
		printf("\tImplicit: %f x + %f y + %f = 0\n", norm.x, norm.y, -1*(norm.x*p1.x + norm.y*p1.y));
		printf("\tParametric: r(t) = (%f, %f) + (%f, %f)t\n", p1.x, p1.y, p2.x-p1.x, p2.y-p1.y);		
	}

	// returns a point of the line at t
	vec3 r(float t){
		return vec3(p1.x + (p2.x-p1.x)*t, p1.y + (p2.y-p1.y)*t);
	}
	vec3 getFirstBoundaryPoint(){
		return r(tMin());
	}
	vec3 getSecondBoundaryPoint(){
		return r(tMax());
	}
};

class LineCollection : Object{
	std::vector<Line> lines;
public:
	LineCollection(){}
	
	void addLine(Line line){
		lines.push_back(line);
		// add vertices of line to vertexCoords based of parametric equation in the 
		// calculate the vertices for current line in the (-1, -1) (1, 1) box
		vec3 first = line.getFirstBoundaryPoint();
		vec3 second = line.getSecondBoundaryPoint();
		// add these two vertices that define the line
		addVertex(first.x, first.y);
		addVertex(second.x, second.y);
	}
	
	void drawLines(){
		draw(GL_LINES, vec3(0.0f, 1.0f, 1.0f));
	}
};

// Objects
PointCollection* pc;
LineCollection* lc;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glLineWidth(3.0f);
	glPointSize(10.0f);

	// Init classes here etc
	pc = new PointCollection();
	lc = new LineCollection();

	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// clear frame buffer

	// Display Points, Lines with their current states 
	pc->drawPoints();
	lc->drawLines();

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
	// Choose the program state
	if (key == 'p') {
		currentState = ProgramState::STATE_P;
		printf("Current state is set to P\n");
	}
	if (key == 'l'){
		currentState = ProgramState::STATE_L;
		printf("Current state is set to L\n");
	} 
	if (key == 'm'){
		currentState = ProgramState::STATE_M;
		printf("Current state is set to M\n");
	} 
	if (key == 'i'){
		currentState = ProgramState::STATE_I;
		printf("Current state is set to I\n");
	}
} 

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}


// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	//printf("Current state is: %s\n", programStateToString(currentState));
	if (currentState == ProgramState::STATE_P){
		if (state == GLUT_UP){
				pc->addPoint(cX, cY);
				glutPostRedisplay();
		} 
	} else if (currentState == ProgramState::STATE_L){
		if (state == GLUT_UP){
				vec3 returnedPoint = pc->getClosestPoint(cX, cY);
				if (!isPointInvalid(returnedPoint)){ // if the clicked point is close to a point and it is returned
					if (isPointInvalid(clickedPoints[0])){ // if the first clicked point is not yet initialized
						clickedPoints[0] = returnedPoint;
					} else if (isPointInvalid(clickedPoints[1])){ // if the second clicked point is not yet initialized
						clickedPoints[1] = returnedPoint;
						if (clickedPoints[1].x == clickedPoints[0].x && clickedPoints[1].y == clickedPoints[0].y){ // if the second clicked point is same as first
							resetClickedPoints();
						}
					}
				} else{
					resetClickedPoints(); // if one of the clicks is invalid then reset the process
				}
				if (!isPointInvalid(clickedPoints[0]) && !isPointInvalid(clickedPoints[1])){ // both point was successfully clicked
					Line line = Line(clickedPoints[0].x, clickedPoints[0].y, clickedPoints[1].x, clickedPoints[1].y); // create the line
					lc->addLine(line);
					resetClickedPoints();
				}
				//printf("Clicked points validity: point1 %d, point2 %d\n", isPointInvalid(clickedPoints[0]), isPointInvalid(clickedPoints[1]));
		} 
	} else if (currentState == ProgramState::STATE_M){
	
	} else if (currentState == ProgramState::STATE_I){
	
	}

	char * buttonStat;
	switch (state) {
	case GLUT_DOWN: buttonStat = "pressed"; break;
	case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
	
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	glutPostRedisplay();
}
