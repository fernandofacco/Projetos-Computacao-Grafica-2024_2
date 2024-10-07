#include <iostream>
#include <string>
#include <assert.h>

#include <vector>
#include <fstream>
#include <sstream>

#include <filesystem>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Classe gerenciadora de shaders
#include "Shader.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

void userKeyInput(GLFWwindow* window);

struct Object {
    GLuint VAO; // Índice do buffer de geometria
    int nVertices; // Número de vértices
    glm::mat4 model; // Matriz de transformações do objeto
};

// Protótipos das funções
int setupGeometry();
int loadSimpleOBJ(string filePATH, int &nVertices);
void renderObjects(const std::vector<Object>& objects, Shader& shader, float angle, GLint modelLoc);
void loadObjectsFromFolder(const std::string& folderPath);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1920, HEIGHT = 1080;

bool rotateX=false, rotateY=false, rotateZ=false;

//Variáveis globais da câmera
glm::vec3 cameraPos = glm::vec3(0.0f,0.0f,3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f,0.0,-1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f,1.0f,0.0f);

bool firstMouse = true;
float yawVariable = -90.0f;
float pitchVariable = 0.0f;
float lastX = 1920 / 2.0;
float lastY = 1080 / 2.0;

bool cursorEnabled = false;

std::vector<Object> objects;

int selectedObjectIndex = -1;

float scaleFactor = 1.0f; // Variável de escala

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D --- Fernando Facco Rodrigues e Luis Henrique Daltoe Dorr!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);

    glfwSetKeyCallback(window, key_callback);

	// Desabilita o cursor do mouse na janela
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	Shader shader("phong.vs","phong.fs");
	std::string folderPath = "./obj/";

    // Carrega todos os objetos da pasta
    loadObjectsFromFolder(folderPath);
	glUseProgram(shader.ID);

	//Matriz de modelo
	glm::mat4 model = glm::mat4(1); //matriz identidade;
	GLint modelLoc = glGetUniformLocation(shader.ID, "model");
	model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//Matriz de view
	glm::mat4 view = glm::lookAt(cameraPos,glm::vec3(0.0f,0.0f,0.0f),cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	//Matriz de projeção
	//glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(39.6f),(float)WIDTH/HEIGHT,0.1f,100.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glEnable(GL_DEPTH_TEST);

	//Propriedades da superfície
	shader.setFloat("ka",0.2);
	shader.setFloat("ks", 1);
	shader.setFloat("kd", 1);
	shader.setFloat("q", 10.0);

	//Propriedades da fonte de luz
	shader.setVec3("lightPos", 50.0f, 10.0f, 50.0f);
	shader.setVec3("lightColor", 1.0, 1.0, 1.0);  // Aumentar a intensidade

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f); // cor de fundo cinza
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();

		renderObjects(objects, shader, angle, modelLoc);

		// Troca os buffers da tela
		glfwSwapBuffers(window);

		userKeyInput(window);
	}

	// Pede pra OpenGL desalocar os buffers
	for (int i = 0; i < objects.size(); i ++) {
		glDeleteVertexArrays(1, &objects[i].VAO);
	}
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

void renderObjects(const std::vector<Object>& objects, Shader& shader, float angle, GLint modelLoc) {
	for (int i = 0; i < objects.size(); i ++) {
		Object obj = objects[i];

		if (i == selectedObjectIndex || selectedObjectIndex == -1) {
            if (rotateX) {
                obj.model = glm::rotate(obj.model, angle, glm::vec3(1.0f, 0.0f, 0.0f));    
            }
            else if (rotateY) {    
                obj.model = glm::rotate(obj.model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            else if (rotateZ) {
                obj.model = glm::rotate(obj.model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
            }
			obj.model = glm::scale(obj.model, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
        }

		if (i == 0) {
            obj.model = glm::translate(obj.model, glm::vec3(-3.0f, 0.0f, 0.0f));
        } else if (i == 1) {
            obj.model = glm::translate(obj.model, glm::vec3(3.0f, 0.0f, 0.0f));
        }

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(obj.model));

		glm::mat4 view = glm::lookAt(cameraPos,cameraPos + cameraFront,cameraUp);
		glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		
		//Propriedades da câmera
		shader.setVec3("cameraPos",cameraPos.x, cameraPos.y, cameraPos.z);

		// Chamada de desenho - drawcall
		// Poligono Preenchido - GL_TRIANGLES
		glBindVertexArray(obj.VAO);
		glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
	}
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void userKeyInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}

	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = false;
	}

	//Verifica a movimentação da câmera
	float cameraSpeed = 0.02f;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        // Alterna o estado do cursor
        cursorEnabled = !cursorEnabled;

        if (cursorEnabled) {
            // Habilita o cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            // Desabilita o cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

	if (action == GLFW_PRESS) { // Verifica se a tecla foi pressionada
		if (key == GLFW_KEY_0){
			selectedObjectIndex = -1;
		}
		else{
			for (int i = 0; i < 9; i++) {
				if (key == GLFW_KEY_1 + i) {
					selectedObjectIndex = i; // Seleciona o objeto correspondente
					break;
				}
			}
		}
    }

	if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) {
        scaleFactor += 0.1f;
    } else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) {
        scaleFactor -= 0.1f;
        if (scaleFactor < 0.1f) scaleFactor = 0.1f; // Previne escala negativa ou zero
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
	float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yawVariable += xoffset;
    pitchVariable += yoffset;

    // make sure that when pitchVariable is out of bounds, screen doesn't get flipped
    if (pitchVariable > 89.0f)
        pitchVariable = 89.0f;
    if (pitchVariable < -89.0f)
        pitchVariable = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yawVariable)) * cos(glm::radians(pitchVariable));
    front.y = sin(glm::radians(pitchVariable));
    front.z = sin(glm::radians(yawVariable)) * cos(glm::radians(pitchVariable));
    cameraFront = glm::normalize(front);
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a 
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {

		//Base da pirâmide: 2 triângulos
		//x    y    z    r    g    b
		-0.5, -0.5, -0.5, 1.0, 1.0, 0.0,
		-0.5, -0.5,  0.5, 0.0, 1.0, 1.0,
		 0.5, -0.5, -0.5, 1.0, 0.0, 1.0,

		 -0.5, -0.5, 0.5, 1.0, 1.0, 0.0,
		  0.5, -0.5,  0.5, 0.0, 1.0, 1.0,
		  0.5, -0.5, -0.5, 1.0, 0.0, 1.0,

		 //
		 -0.5, -0.5, -0.5, 1.0, 1.0, 0.0,
		  0.0,  0.5,  0.0, 1.0, 1.0, 0.0,
		  0.5, -0.5, -0.5, 1.0, 1.0, 0.0,

		  -0.5, -0.5, -0.5, 1.0, 0.0, 1.0,
		  0.0,  0.5,  0.0, 1.0, 0.0, 1.0,
		  -0.5, -0.5, 0.5, 1.0, 0.0, 1.0,

		   -0.5, -0.5, 0.5, 1.0, 1.0, 0.0,
		  0.0,  0.5,  0.0, 1.0, 1.0, 0.0,
		  0.5, -0.5, 0.5, 1.0, 1.0, 0.0,

		   0.5, -0.5, 0.5, 0.0, 1.0, 1.0,
		  0.0,  0.5,  0.0, 0.0, 1.0, 1.0,
		  0.5, -0.5, -0.5, 0.0, 1.0, 1.0,


	};

	GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);


	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

void loadObjectsFromFolder(const std::string& folderPath) {    
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".obj") {
            // Cria um novo objeto para cada arquivo .obj
            Object obj;
            obj.VAO = loadSimpleOBJ(entry.path().string(), obj.nVertices);
			obj.model = glm::mat4(1); //matriz identidade 
			
            // Adiciona o objeto ao vetor
            objects.push_back(obj);
        }
    }
	std::cout << "Total de objetos carregados: " << objects.size() << std::endl;
}

int loadSimpleOBJ(string filePath, int &nVertices)
{
	vector <glm::vec3> vertices;
	vector <glm::vec2> texCoords;
	vector <glm::vec3> normals;
	vector <GLfloat> vBuffer;

	glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);

	ifstream arqEntrada;

	arqEntrada.open(filePath.c_str());
	if (arqEntrada.is_open())
	{
		//Fazer o parsing
		string line;
		while (!arqEntrada.eof())
		{
			getline(arqEntrada,line);
			istringstream ssline(line);
			string word;
			ssline >> word;
			if (word == "v")
			{
				glm::vec3 vertice;
				ssline >> vertice.x >> vertice.y >> vertice.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				vertices.push_back(vertice);

			}
			if (word == "vt")
			{
				glm::vec2 vt;
				ssline >> vt.s >> vt.t;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				texCoords.push_back(vt);

			}
			if (word == "vn")
			{
				glm::vec3 normal;
				ssline >> normal.x >> normal.y >> normal.z;
				//cout << vertice.x << " " << vertice.y << " " << vertice.z << endl;
				normals.push_back(normal);

			}
			else if (word == "f")
			{
				while (ssline >> word) 
				{
					int vi, ti, ni;
					istringstream ss(word);
    				std::string index;

    				// Pega o índice do vértice
    				std::getline(ss, index, '/');
    				vi = std::stoi(index) - 1;  // Ajusta para índice 0

    				// Pega o índice da coordenada de textura
    				std::getline(ss, index, '/');
    				ti = std::stoi(index) - 1;

    				// Pega o índice da normal
    				std::getline(ss, index);
    				ni = std::stoi(index) - 1;

					//Recuperando os vértices do indice lido
					vBuffer.push_back(vertices[vi].x);
					vBuffer.push_back(vertices[vi].y);
					vBuffer.push_back(vertices[vi].z);
					
					//Atributo cor
					vBuffer.push_back(color.r);
					vBuffer.push_back(color.g);
					vBuffer.push_back(color.b);

					//Atributo coordenada de textura
					vBuffer.push_back(texCoords[ti].s);
					vBuffer.push_back(texCoords[ti].t);

					//Atributo vetor normal
					vBuffer.push_back(normals[ni].x);
					vBuffer.push_back(normals[ni].y);
					vBuffer.push_back(normals[ni].z);
					
        			
        			// Exibindo os índices para verificação
       				// std::cout << "v: " << vi << ", vt: " << ti << ", vn: " << ni << std::endl;
    			}
				
			}
		}

		arqEntrada.close();

		cout << "Gerando o buffer de geometria..." << endl;
		GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Atributo coordenada de textura - s, t
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	//Atributo vetor normal - x, y, z
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8*sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 2;
	return VAO;

	}
	else
	{
		cout << "Erro ao tentar ler o arquivo " << filePath << endl;
		return -1;
	}
}
