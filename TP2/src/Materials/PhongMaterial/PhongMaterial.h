

#ifndef _PhongMaterial_
#define _PhongMaterial_


#include "MaterialGL.h"




struct Phong {
	glm::vec4 coeff;
	glm::vec3 albedo;
	glm::vec3 specularColor;
};




class PhongMaterial : public MaterialGL
{
public:

	//Attributs

	//Constructeur-Destructeur

	/**
		Constructeur de la classe à partir du nom du matériau
		@param name : nom du matériau
	*/
	PhongMaterial(string name = "");

	/**
		Destructeur de la classe
	*/
	~PhongMaterial();

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


protected:
	GLProgram* vp;
	GLProgram* fp;

	GLuint l_ViewProj, l_Model, l_PosLum, l_PosCam,l_Phong,l_Albedo,l_specColor, l_Time;
	Phong param;

};

#endif