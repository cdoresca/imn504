
#include "MaterialRotation.h"
#include "Node.h"
#include <glm/gtc/type_ptr.hpp>


MaterialRotation::MaterialRotation(std::string name) :
	MaterialGL(name)
{
	
	rot = false;
	rspeed = 0.005f;
		
}

MaterialRotation::~MaterialRotation()
{
	
}

void MaterialRotation::render(Node *o)
{

	

}
void MaterialRotation::animate(Node* o, const float elapsedTime)
{
	
	if (rot)
		o->frame()->rotate(glm::vec3(0.0, 1.0, 0.0), glm::radians(elapsedTime * rspeed));
	

}

void MaterialRotation::displayInterface()
{
	ImGui::Checkbox("Enable Rotation", &rot);
	ImGui::SliderFloat("Rotation Speed", &rspeed, 0.0f, 0.2f);

}