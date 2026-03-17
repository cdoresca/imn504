

#pragma once


#include "MaterialGL.h"

#include "CustomModelGL.h"
#include "Texture2D.h"

constexpr auto PARTICULENUMBER = 10000;
const int CELL_COUNT = 34 * 34 * 34;


class ParticuleMaterial : public MaterialGL
{
	struct Phong {
		glm::vec4 coeff;
		glm::vec3 albedo;
		glm::vec3 specularColor;
	};


	struct physiks
	{
        float deltaTime;
        float mass;
        float gravity;
        float _pad;
	};


public:

	//Attributs

	//Constructeur-Destructeur

	/**
		Constructeur de la classe à partir du nom du matériau
		@param name : nom du matériau
	*/
	ParticuleMaterial(string name);

	/**
		Destructeur de la classe
	*/
	~ParticuleMaterial();

	//Méthodes

	/**
		Méthode virtuelle qui est appelée pour faire le rendu d'un objet en utilisant ce matériau
		@param o : Node/Objet pour lequel on veut effectuer le rendu
	*/
	virtual void render(Node* o);

	/**
		Méthode virtuelle qui est appelée pour modifier une valeur d'un paramètre nécessaire pour le rendu
		@param o : Node/Objet concerné par le rendu
		@param elapsedTime : temps
	*/
	virtual void animate(Node* o, const float elapsedTime);



	 void updatePhong();

	virtual void displayInterface() ;

	void simulation();

	void updateSimulationParameters();

protected:
	GLProgram* vp;
	GLProgram* fp;

	GLProgram* cp;
	GLProgram* countCellp;
    GLProgram* sortp;

    GLuint mQueryTimeElapsed;
    GLuint64 mSimTime;

	glm::dvec3 up_direction;

	GLuint l_ViewProj, l_Model, l_PosLum, l_PosCam, l_Phong, l_Albedo, l_specColor, l_Time;

	GLuint lDeltaTime, lMass, l_GravityDir, l_PNum;

    GLuint m_Positions[2];

     GLuint m_Velocities[2];

    GLuint m_Colors, m_Count, m_Start, m_StartSim, m_ParticuleID;

	GLuint physic;

	Phong param;

	int mode;

	uint32_t start[CELL_COUNT];


	/// physiks
    physiks physik;


	/**
		Métode qui initialise le tableau uint32_t start en calculant leur offset de façon séquentielle sur cpu
	*/
	void prefixSum();

};
