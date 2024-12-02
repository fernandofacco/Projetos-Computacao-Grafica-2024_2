#include <iostream>
#include <string>
#include <assert.h>

#include <vector>
#include <fstream>
#include <sstream>

#include <filesystem>

#include <unordered_map>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Classe gerenciadora de shaders
#include "Shader.h"

// Biblioteca JSON
#include "json.hpp"

struct Curve
{
    std::vector<glm::vec3> controlPoints; // Pontos de controle da curva
    std::vector<glm::vec3> curvePoints;   // Pontos da curva
    glm::mat4 M;                          // Matriz dos coeficientes da curva
};

struct Object
{
	GLuint VAO; //Índice do buffer de geometria
	GLuint texID; //Identificador da textura carregada
	int nVertices; //nro de vértices
	glm::mat4 model; //matriz de transformações do objeto
	float ka, kd, ks; //coeficientes de iluminação - material do objeto
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;
	Curve curve;
	int curveIndex = 0;
	float curveLastTime = 0.0;
	float curveFPS = 60.0;
	float curveAngle = 0.0;
};

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

void userKeyInput(GLFWwindow* window);

// Protótipos das funções
int loadSimpleOBJ(string filePATH, int &nVertices);
GLuint loadTexture(string filePATH, int &width, int &height);
void loadMTL(string filePATH, Object &obj);
void renderObjects(Shader& shader, float angle, GLint modelLoc);
void loadSceneConfig(string filePATH);
void loadObjectsFromFolder(string objFolderPath, string textureFolderPath, string mtlFolderPath, Curve& curve);

std::vector<glm::vec3> generateInfinityControlPoints(int numPoints = 20);
std::vector<glm::vec3> generateCircleControlPoints(int numPoints = 20);
void generateGlobalBezierCurvePoints(Curve &curve, int numPoints);  

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1920, HEIGHT = 1080;

//Variáveis globais da câmera
glm::vec3 cameraPos;
glm::vec3 cameraFront;
glm::vec3 cameraUp;

glm::vec3 lightPos;
glm::vec3 lightColor;

bool firstMouse = true;
float yawVariable = -90.0f;
float pitchVariable = 0.0f;
float lastX = 1920 / 2.0;
float lastY = 1080 / 2.0;

bool cursorEnabled = false;

std::vector<Object> objects;

int selectedObjectIndex = -1;

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

	// Estrutura para armazenar a curva de Bézier e pontos de controle
    Curve curvaBezier;

    curvaBezier.controlPoints = generateInfinityControlPoints();

    // Gerar pontos da curva de Bézier
    int numCurvePoints = 100; // Quantidade de pontos por segmento na curva
    generateGlobalBezierCurvePoints(curvaBezier, numCurvePoints);

	std::string sceneJsonFilePath = "./sceneConfig.json";
	loadSceneConfig(sceneJsonFilePath);
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
	glm::mat4 projection = glm::perspective(glm::radians(39.6f),(float)WIDTH/HEIGHT,0.1f,100.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//Buffer de textura no shader
	glUniform1i(glGetUniformLocation(shader.ID, "texBuffer"), 0);

	glEnable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);

	//Propriedades da superfície
	shader.setFloat("ka",0.2);
	shader.setFloat("ks", 1);
	shader.setFloat("kd", 1);
	shader.setFloat("q", 10.0);

	//Propriedades da fonte de luz
	shader.setVec3("lightPos", lightPos.x, lightPos.y, lightPos.z);
	shader.setVec3("lightColor", lightColor.r, lightColor.g, lightColor.b);  // Aumentar a intensidade

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

		renderObjects(shader, angle, modelLoc);

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

void renderObjects(Shader& shader, float angle, GLint modelLoc) {
    for (Object& obj : objects) {
        obj.model = glm::mat4(1.0f);
		
		if (!obj.curve.curvePoints.empty()){
			obj.position = obj.curve.curvePoints[obj.curveIndex];

			// Incrementando o índice do frame apenas quando fechar a taxa de FPS desejada
			auto now = glfwGetTime();
			auto dt = now - obj.curveLastTime;

			if (dt >= 1 / obj.curveFPS)
			{
				obj.curveIndex = (obj.curveIndex + 1) % obj.curve.curvePoints.size(); // incrementando ciclicamente o indice do Frame
				obj.curveLastTime = now;
				glm::vec3 nextPos = obj.curve.curvePoints[obj.curveIndex];
				glm::vec3 dir = glm::normalize(nextPos - obj.position);
				obj.curveAngle = atan2(dir.y, dir.x) + glm::radians(-90.0f);
			}
		}

		obj.model = glm::translate(obj.model, obj.position);
		obj.model = glm::scale(obj.model, glm::vec3(obj.scale.x, obj.scale.y, obj.scale.z));
		
		if (obj.rotation.x != 0 || obj.rotation.y != 0 || obj.rotation.z != 0){
			obj.model = glm::rotate(obj.model, angle, glm::vec3(obj.rotation.x, obj.rotation.y, obj.rotation.z));
		}

		shader.setFloat("ka", obj.ka);
		shader.setFloat("ks", obj.ks);
		shader.setFloat("kd", obj.kd);

        // Atualizar a matriz de modelo no shader
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(obj.model));

        // Atualizar a matriz de visão no shader
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // Propriedades da câmera
        shader.setVec3("cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);

        // Chamada de desenho - drawcall
        // Poligono Preenchido - GL_TRIANGLES
        glBindVertexArray(obj.VAO);
		glBindTexture(GL_TEXTURE_2D,obj.texID);
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
		objects[selectedObjectIndex].rotation.x = 1;
	}

	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
	{
		objects[selectedObjectIndex].rotation.y = 1;
	}

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
	{
		objects[selectedObjectIndex].rotation.z = 1;
	}

	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
	{
		objects[selectedObjectIndex].rotation.x = 0;
		objects[selectedObjectIndex].rotation.y = 0;
		objects[selectedObjectIndex].rotation.z = 0;
	}

	/*if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
	{
		objects[selectedObjectIndex].isCurve = !objects[selectedObjectIndex].isCurve;
	}*/

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

	// Movimenta o objeto selecionado com as setas do teclado
    float movementSpeed = 0.005f; // Velocidade de movimentação

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		objects[selectedObjectIndex].position.y += movementSpeed; // Move para cima	
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		objects[selectedObjectIndex].position.y -= movementSpeed; // Move para baixo
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		objects[selectedObjectIndex].position.x -= movementSpeed; // Move para a esquerda
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		objects[selectedObjectIndex].position.x += movementSpeed; // Move para a direita
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
		objects[selectedObjectIndex].scale += 0.1f;
	} else if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) {
		objects[selectedObjectIndex].scale -= 0.1f;

		// Previne escala menor que 0.1 para todos os componentes do vetor
		if (objects[selectedObjectIndex].scale.x < 0.1f) {
			objects[selectedObjectIndex].scale.x = 0.1f;
		}
		if (objects[selectedObjectIndex].scale.y < 0.1f) {
			objects[selectedObjectIndex].scale.y = 0.1f;
		}
		if (objects[selectedObjectIndex].scale.z < 0.1f) {
			objects[selectedObjectIndex].scale.z = 0.1f;
		}
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


void loadSceneConfig(string filePATH){
    std::ifstream inputFile(filePATH);
    if (!inputFile.is_open()) {
        std::cerr << "Erro ao abrir o arquivo JSON: " << filePATH << std::endl;
        return;
    }

    // Parse do JSON
	nlohmann::json jsonSceneConfig;
    inputFile >> jsonSceneConfig;

    // Carregar objetos
    if (jsonSceneConfig.contains("objects")) {
        for (const auto& objData : jsonSceneConfig["objects"]) {
            std::string objFile = objData["objFile"];
            std::string textureFile = objData["textureFile"];
            std::string mtlFile = objData["mtlFile"];
            
            glm::vec3 position = glm::vec3(
                objData["position"][0],
                objData["position"][1],
                objData["position"][2]
            );

            glm::vec3 scale = glm::vec3(
                objData["scale"][0],
                objData["scale"][1],
                objData["scale"][2]
            );

            glm::vec3 rotation = glm::vec3(
                objData["rotation"][0],
                objData["rotation"][1],
                objData["rotation"][2]
            );

            // Criar objeto e configurar propriedades
			Object obj;
			obj.VAO = loadSimpleOBJ(objFile, obj.nVertices);
			obj.model = glm::mat4(1); //matriz identidade 
			obj.position = position;
			obj.scale = scale;
			obj.rotation = rotation;

            if (objData.contains("curveAnimation") && (objData["curveAnimation"] == "infinity" || objData["curveAnimation"] == "circle")) {
				Curve curvaBezier;

                if (objData["curveAnimation"] == "infinity"){
					curvaBezier.controlPoints = generateInfinityControlPoints();
				} else if (objData["curveAnimation"] == "circle"){
					curvaBezier.controlPoints = generateCircleControlPoints();
				}

				// Gerar pontos da curva de Bézier
				int numCurvePoints = 100; // Quantidade de pontos por segmento na curva
				generateGlobalBezierCurvePoints(curvaBezier, numCurvePoints);

				obj.curve = curvaBezier;
				glm::vec3 nextPos = obj.curve.curvePoints[1];
				glm::vec3 dir = glm::normalize(nextPos - obj.position);
				obj.curveAngle = atan2(dir.y, dir.x) + glm::radians(-90.0f);
            }


			if (std::filesystem::exists(textureFile)) {
                int texWidth, texHeight;
                obj.texID = loadTexture(textureFile, texWidth, texHeight);
                std::cout << "Textura carregada para " << objFile << ": " << textureFile << std::endl;
            } else {
                std::cerr << "Textura não encontrada para " << objFile << std::endl;
                obj.texID = 0; // Identificador inválido para textura
            }

            if (std::filesystem::exists(mtlFile)) {
				loadMTL(mtlFile, obj);
            } else {
                std::cerr << "Arquivo MTL não encontrado para " << objFile << std::endl;
            }
			
            // Adiciona o objeto ao vetor
            objects.push_back(obj);
        }
    }

    // Configurar luz
    if (jsonSceneConfig.contains("light")) {
        lightPos = glm::vec3(
            jsonSceneConfig["light"]["lightPos"][0],
            jsonSceneConfig["light"]["lightPos"][1],
            jsonSceneConfig["light"]["lightPos"][2]
        );

        lightColor = glm::vec3(
            jsonSceneConfig["light"]["lightColor"][0],
            jsonSceneConfig["light"]["lightColor"][1],
            jsonSceneConfig["light"]["lightColor"][2]
        );
    }

    // Configurar câmera
    if (jsonSceneConfig.contains("camera")) {
        cameraPos = glm::vec3(
            jsonSceneConfig["camera"]["cameraPos"][0],
            jsonSceneConfig["camera"]["cameraPos"][1],
            jsonSceneConfig["camera"]["cameraPos"][2]
        );

        cameraFront = glm::vec3(
            jsonSceneConfig["camera"]["cameraFront"][0],
            jsonSceneConfig["camera"]["cameraFront"][1],
            jsonSceneConfig["camera"]["cameraFront"][2]
        );

        cameraUp = glm::vec3(
            jsonSceneConfig["camera"]["cameraUp"][0],
            jsonSceneConfig["camera"]["cameraUp"][1],
            jsonSceneConfig["camera"]["cameraUp"][2]
        );
    }

    std::cout << "Cena carregada com sucesso a partir de " << filePATH << std::endl;
}

void loadObjectsFromFolder(string objFolderPath, string textureFolderPath, string mtlFolderPath, Curve& curve) {    
    for (const auto& entry : std::filesystem::directory_iterator(objFolderPath)) {
        if (entry.path().extension() == ".obj") {
            // Cria um novo objeto para cada arquivo .obj
            Object obj;
            obj.VAO = loadSimpleOBJ(entry.path().string(), obj.nVertices);
			obj.model = glm::mat4(1); //matriz identidade 
			//obj.scale = 1.0f; // Escala inicial do objeto para exibir

			// Curva e angulo da curva
			obj.curve = curve;
			glm::vec3 nextPos = obj.curve.curvePoints[1];
			glm::vec3 dir = glm::normalize(nextPos - obj.position);
			obj.curveAngle = atan2(dir.y, dir.x) + glm::radians(-90.0f);

			// Nome do arquivo base (sem extensão)
            std::string baseName = entry.path().stem().string();

			// Busca o arquivo de textura correspondente
            std::string texturePathJpeg = textureFolderPath + "/" + baseName + ".jpeg";
			std::string texturePathJpg = textureFolderPath + "/" + baseName + ".jpg";
			std::string texturePathPng = textureFolderPath + "/" + baseName + ".png";
			std::string texturePath = "";

            if (std::filesystem::exists(texturePathJpeg)) {
				texturePath = texturePathJpeg;
            } else if (std::filesystem::exists(texturePathJpg)){
				texturePath = texturePathJpg;
            } else if (std::filesystem::exists(texturePathPng)){
				texturePath = texturePathPng;
            }

			if (std::filesystem::exists(texturePath)) {
                int texWidth, texHeight;
                obj.texID = loadTexture(texturePath, texWidth, texHeight);
                std::cout << "Textura carregada para " << entry.path().filename() << ": " << texturePath << std::endl;
            } else {
                std::cerr << "Textura não encontrada para " << entry.path().filename() << std::endl;
                obj.texID = 0; // Identificador inválido para textura
            }

			// Busca o arquivo mtl correspondente
            std::string mtlPath = mtlFolderPath + "/" + baseName + ".mtl";

            if (std::filesystem::exists(mtlPath)) {
				loadMTL(mtlPath, obj);
            } else {
                std::cerr << "Arquivo MTL não encontrado para " << entry.path().filename() << std::endl;
            }
			
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

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

void loadMTL(string filePath, Object &obj)
{
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
			if (word == "Kd")
			{
				glm::vec3 kValue;
				ssline >> kValue.x >> kValue.y >> kValue.z;
				obj.kd = (kValue.x + kValue.y + kValue.z) / 3.0f;

			}
			if (word == "Ka")
			{
				glm::vec3 kValue;
				ssline >> kValue.x >> kValue.y >> kValue.z;
				obj.ka = (kValue.x + kValue.y + kValue.z) / 3.0f;

			}
			if (word == "Ks")
			{
				glm::vec3 kValue;
				ssline >> kValue.x >> kValue.y >> kValue.z;
				obj.ks = (kValue.x + kValue.y + kValue.z) / 3.0f;
			}
		}

		arqEntrada.close();

		cout << "Arquivo .mtl lido" << endl;
	}
}

std::vector<glm::vec3> generateInfinityControlPoints(int numPoints)
{
    std::vector<glm::vec3> controlPoints;

    float step = 2 * 3.14159 / (numPoints - 1);

    for (int i = 0; i < numPoints; i++)
    {
        float t = i * step;

        // Fórmulas paramétricas para a lemniscata de Bernoulli
        float width = 2.5;
        float height = 2.5; 
        float denom = 1 + pow(sin(t), 2);
        float x = (width * cos(t)) / denom;
        float y = (height * width * sin(t) * cos(t)) / denom;

        controlPoints.push_back(glm::vec3(x, y, 0.0f));
    }
    controlPoints.push_back(controlPoints[0]);

    return controlPoints;
}

std::vector<glm::vec3> generateCircleControlPoints(int numPoints)
{
    std::vector<glm::vec3> controlPoints;

	float radius = 4.0f;

    float step = 2 * 3.14159f / numPoints;

    for (int i = 0; i < numPoints; i++)
    {
        float t = i * step;

        float x = radius * cos(t);
        float y = radius * sin(t);

        controlPoints.push_back(glm::vec3(x, y, 0.0f));
    }
    controlPoints.push_back(controlPoints[0]);

    return controlPoints;
}

void generateGlobalBezierCurvePoints(Curve &curve, int numPoints)
{
    curve.curvePoints.clear(); // Limpa quaisquer pontos antigos da curva

    int n = curve.controlPoints.size() - 1; // Grau da curva
    float t;
    float piece = 1.0f / (float)numPoints;

    for (int j = 0; j <= numPoints; ++j)
    {
        t = j * piece;
        glm::vec3 point(0.0f); // Ponto na curva

        // Calcula o ponto da curva usando a fórmula de Bernstein
        for (int i = 0; i <= n; ++i)
        {
            // Coeficiente binomial
            float binomialCoeff = (float)(tgamma(n + 1) / (tgamma(i + 1) * tgamma(n - i + 1)));
            // Polinômio de Bernstein
            float bernsteinPoly = binomialCoeff * pow(1 - t, n - i) * pow(t, i);
            // Soma ponderada dos pontos de controle
            point += bernsteinPoly * curve.controlPoints[i];
        }

        curve.curvePoints.push_back(point);
    }
}