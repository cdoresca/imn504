
#include "ParticuleMaterial.h"
#include "Node.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

using namespace glm;

ParticuleMaterial::ParticuleMaterial(std::string name) :
	MaterialGL(name)
{
    

	vp = new GLProgram(MaterialPath + "ParticuleMaterial/ParticuleMaterial-VS.glsl", GL_VERTEX_SHADER);
	fp = new GLProgram(MaterialPath + "ParticuleMaterial/ParticuleMaterial-FS.glsl", GL_FRAGMENT_SHADER);

    cp = new GLProgram(MaterialPath + "ParticuleMaterial/ParticuleMaterial-CS.glsl", GL_COMPUTE_SHADER);
    countCellp = new GLProgram(MaterialPath + "ParticuleMaterial/initGrid.glsl", GL_COMPUTE_SHADER);
    sortp = new GLProgram(MaterialPath + "ParticuleMaterial/Sort.glsl", GL_COMPUTE_SHADER);
    

	m_ProgramPipeline->useProgramStage(vp, GL_VERTEX_SHADER_BIT);
	m_ProgramPipeline->useProgramStage(fp, GL_FRAGMENT_SHADER_BIT);

   
	l_ViewProj = glGetUniformLocation(vp->getId(), "ViewProj");
	l_Model = glGetUniformLocation(vp->getId(), "Model");
	l_PosLum = glGetUniformLocation(vp->getId(), "PosLum");
	l_PosCam = glGetUniformLocation(vp->getId(), "PosCam");



	l_Phong = glGetUniformLocation(fp->getId(), "Phong");
	l_Albedo = glGetUniformLocation(fp->getId(), "diffuseAlbedo");
	l_specColor = glGetUniformLocation(fp->getId(), "specularColor");


	param.albedo = glm::vec3(0.2, 0.7, 0.8);
	param.coeff = glm::vec4(0.2,0.8,1.0,100.0);
	param.specularColor = glm::vec3(1.0);

	glCreateQueries(GL_TIME_ELAPSED, 1, &mQueryTimeElapsed);
    mSimTime = 0;
	
	l_GravityDir = glGetUniformLocation(cp->getId(), "gravityDir");
    l_PNum = glGetUniformLocation(cp->getId(), "PARTICULENUMBER");


	/**************************TP 2 ****************************/
	// Create SSBO  : glCreateBuffers (position,velocité, couleur (?)) 
	// Populate SSB : glNamedBufferStorage - Pensez a utiliser des vec4 pour eviter les erreurs d'alignements
	// Repartir les particules aléatoirement (utiliser glm::sphericalRand)
	// Créer le compute shader

	/**********************************************************/ 
    vec4 pos[PARTICULENUMBER];
    vec4 vitesse[PARTICULENUMBER];
    vec4 color[PARTICULENUMBER];
	for (int i = 0; i < PARTICULENUMBER; i++) {
        pos[i] = vec4(sphericalRand(1.0f), 1.0f);
        vitesse[i] = vec4();
        color[i] = vec4(linearRand(vec3(0), vec3(1)), 1);
	}

	//buffer positions
	glCreateBuffers(2,m_Positions);
    glNamedBufferData(m_Positions[0], sizeof(pos), pos, GL_DYNAMIC_COPY);
    glNamedBufferData(m_Positions[1], sizeof(pos), nullptr, GL_DYNAMIC_COPY);

	// buffer vitesse
	glCreateBuffers(2, m_Velocities);
    glNamedBufferData(m_Velocities[0], sizeof(vitesse), vitesse, GL_DYNAMIC_COPY);
    glNamedBufferData(m_Velocities[1], sizeof(vitesse), nullptr, GL_DYNAMIC_COPY);

	
	physik.mass = 0.1f;
    physik.deltaTime = 0.01f;
    physik.gravity = 9.8f;
 
	// buffer données physiques
	glCreateBuffers(1, &physic);
    glNamedBufferData(physic, sizeof(physiks), &physik, GL_DYNAMIC_COPY);

	//buffer couleur
	glCreateBuffers(1, &m_Colors);
    glNamedBufferData(m_Colors, sizeof(color), color, GL_DYNAMIC_COPY);

	//buffer nombre de particule dans chaque cellule
	glCreateBuffers(1, &m_Count);
    glNamedBufferData(m_Count, sizeof(uint32_t) * CELL_COUNT, nullptr, GL_DYNAMIC_COPY);

	// buffer offset des cellule
	glCreateBuffers(1, &m_Start);
    glNamedBufferData(m_Start, sizeof(uint32_t) * CELL_COUNT, nullptr, GL_DYNAMIC_COPY);

	// Copie de start
	glCreateBuffers(1, &m_StartSim);
    glNamedBufferData(m_StartSim, sizeof(uint32_t) * CELL_COUNT, nullptr, GL_DYNAMIC_COPY);

	// buffer tableau d'indice des particules mis en ordre selon les indices des cellules
	glCreateBuffers(1, &m_ParticuleID);
    glNamedBufferData(m_ParticuleID, sizeof(uint32_t) * PARTICULENUMBER, nullptr, GL_DYNAMIC_COPY);

	l_Time = glGetUniformLocation(vp->getId(), "Time");


	updateSimulationParameters();
	updatePhong();
}

ParticuleMaterial::~ParticuleMaterial()
{

}

void ParticuleMaterial::render(Node* o)
{

	/**************************TP 2 ****************************/
	// Lier les SSBO pour le rendu : glBindBufferBase
	/**********************************************************/
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Positions[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,m_Colors);

	m_ProgramPipeline->bind();

	// Afficher en utilisant l'instanciation - Affiche N modeles
	 o->drawGeometryInstanced(GL_TRIANGLES,PARTICULENUMBER);

	m_ProgramPipeline->release();
}
void ParticuleMaterial::animate(Node* o, const float elapsedTime)
{

	glm::mat4 viewproj = Scene::getInstance()->camera()->getProjectionMatrix() * Scene::getInstance()->camera()->getViewMatrix();

	glProgramUniformMatrix4fv(vp->getId(), l_ViewProj, 1, GL_FALSE, glm::value_ptr(viewproj));
	glProgramUniformMatrix4fv(vp->getId(), l_Model, 1, GL_FALSE, glm::value_ptr(o->frame()->getModelMatrix()));
	glProgramUniform3fv(vp->getId(), l_PosLum, 1,  glm::value_ptr(Scene::getInstance()->getNode("Light")->frame()->convertPtTo(glm::vec3(0.0,0.0,0.0),o->frame())));
	glProgramUniform3fv(vp->getId(), l_PosCam, 1, glm::value_ptr(Scene::getInstance()->camera()->frame()->convertPtTo(glm::vec3(0.0, 0.0, 0.0), o->frame())));

	auto now_time = std::chrono::high_resolution_clock::now();
	auto timevar = now_time.time_since_epoch();
	float millis = 0.001f*std::chrono::duration_cast<std::chrono::milliseconds>(timevar).count();
	
	/*Direction du vecteur up dans le rep�re de l'objet. A utiliser pour d�finir la direction de la force de gravit�*/
	glm::vec3 gravityDir = Scene::getInstance()->getSceneNode()->frame()->convertDirTo(glm::vec3(0.0, -1.0, 0.0), o->frame());
    /**************************TP 2 ****************************/
    // Envoyer la direction de la gravité au compute shader
    
    /**********************************************************/

    glProgramUniform3fv(cp->getId(), l_GravityDir,1 ,glm::value_ptr(gravityDir));
    glProgramUniform1ui(cp->getId(), l_PNum, PARTICULENUMBER);
    glProgramUniform1ui(countCellp->getId(), l_PNum, PARTICULENUMBER);
    glProgramUniform1ui(sortp->getId(), l_PNum, PARTICULENUMBER);

	simulation();
	

}

void ParticuleMaterial::simulation()
{
    uint32_t zero = 0;
    glClearNamedBufferData(m_Count, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

	glBeginQuery(GL_TIME_ELAPSED, mQueryTimeElapsed);

		/**************************TP 2 ****************************/
        // Lier les SSBOs : glBindBufferBase
        // Lancer le compute shader :  glDispatchCompute(X,Y,Z);

        /**********************************************************/
      
    GLuint local_size = 64;
    GLuint numGroups = (PARTICULENUMBER + local_size - 1) / local_size;
    
	// initialise tableau count
    glUseProgram(countCellp->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Positions[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_Count);
    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//partie cpu offset
	prefixSum();

	//copy start
    glCopyNamedBufferSubData(m_Start, m_StartSim, 0, 0, CELL_COUNT * sizeof(uint32_t));


	//sort
	glUseProgram(sortp->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Positions[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_Start); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_ParticuleID);
    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	
	//simulation collision particule + box + gravité
    glUseProgram(cp->getId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_Positions[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_Positions[1]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_Velocities[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_Velocities[1]);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, physic);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_Count);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_StartSim);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7,m_ParticuleID);
    numGroups = (PARTICULENUMBER + local_size - 1) / local_size;
    glDispatchCompute(numGroups, 1, 1);
   
	glUseProgram(0);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glEndQuery(GL_TIME_ELAPSED);


    GLuint64 result = static_cast<GLuint64>(0);

	std::swap(m_Positions[0], m_Positions[1]);
    std::swap(m_Velocities[0], m_Velocities[1]);
    glGetQueryObjectui64v(mQueryTimeElapsed, GL_QUERY_RESULT, &result);
    mSimTime = result;

}


void ParticuleMaterial::updatePhong()
{
	glProgramUniform4fv(fp->getId(), l_Phong, 1, glm::value_ptr(glm::vec4(param.coeff)));
	glProgramUniform3fv(fp->getId(), l_Albedo, 1, glm::value_ptr(param.albedo));
	glProgramUniform3fv(fp->getId(), l_specColor, 1, glm::value_ptr(param.specularColor));
}



void ParticuleMaterial::updateSimulationParameters()
{
    /**************************TP 2 ****************************/
    // Mettre a jour les parmetres de la simulation

    /**********************************************************/
    glNamedBufferSubData(physic, 0, sizeof(physiks), &physik);
}

void ParticuleMaterial::prefixSum() {

	uint32_t *count = (uint32_t *)glMapNamedBuffer(m_Count, GL_READ_ONLY);

    uint32_t acc = 0;
    for (uint32_t c = 0; c < CELL_COUNT; c++) {
        start[c] = acc;
        acc += count[c];
    }

	glUnmapNamedBuffer(m_Count);

	glNamedBufferSubData(m_Start, 0, CELL_COUNT * sizeof(uint), start);
}




void ParticuleMaterial::displayInterface()
{

	if (ImGui::TreeNode("Physical parameters"))
	{
        ImGui::BeginGroup();

        ImGui::Text("Simulaion time : %f ms/frame", (mSimTime * 1.e-6));
			
			bool upd = false;
            upd = ImGui::SliderFloat("Particule", &physik.mass, 0.1f, 10.0f, "Mass = %.3f") || upd;
            upd = ImGui::SliderFloat("deltaTime", &physik.deltaTime, 0.0f, 0.2f, "DeltaTime = %.3f") || upd;
			
			ImGui::EndGroup();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::TreePop();

			if (upd)
                updateSimulationParameters();
	}
	if (ImGui::TreeNode("PhongParameters"))
	{
	bool upd = false;
		upd = ImGui::SliderFloat("ambiant", &param.coeff.x, 0.0f, 1.0f, "ambiant = %.2f") || upd;
		upd = ImGui::SliderFloat("diffuse", &param.coeff.y, 0.0f, 1.0f, "diffuse = %.2f") || upd;
		upd = ImGui::SliderFloat("specular", &param.coeff.z, 0.0f, 2.0f, "specular = %.2f") || upd;
		upd = ImGui::SliderFloat("exposant", &param.coeff.w, 0.1f, 200.0f, "exposant = %f") || upd;
		ImGui::PushItemWidth(200.0f);
		upd = ImGui::ColorPicker3("Albedo", glm::value_ptr(param.albedo)) || upd;;
		ImGui::SameLine();
		upd = ImGui::ColorPicker3("Specular Color", glm::value_ptr(param.specularColor)) || upd;;
		ImGui::PopItemWidth();
		if (upd)
			updatePhong();
		ImGui::TreePop();
	}


}




