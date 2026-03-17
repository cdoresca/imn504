#include "CustomModelGL.h"


CustomModelGL::CustomModelGL(std::string name, int _nbElements){
    this->m_Name = name;

    m_nbElements = _nbElements;

    m_Model = new GeometricModel();

    createDeformableGrid();
}


void CustomModelGL::createDeformableGrid()
{
	for (float i = 0; i < m_nbElements; i++)
		for (float j = 0; j < m_nbElements; j++)
		{
			
			if (i < m_nbElements - 1 && j < m_nbElements - 1)
			{
				Face f;
				f.s3 = (int)m_Model->listVertex.size();
				f.s2 = (int)m_Model->listVertex.size() + 1;
				f.s1 = (int)m_Model->listVertex.size() + 1 + m_nbElements;
				m_Model->listFaces.push_back(f);
				m_Model->listCoordFaces.push_back(f);

				f.s3 = (int)m_Model->listVertex.size();
				f.s2 = (int)m_Model->listVertex.size() + 1 + m_nbElements;
				f.s1 = (int)m_Model->listVertex.size() + m_nbElements;
				m_Model->listFaces.push_back(f);
				m_Model->listCoordFaces.push_back(f);


			}
			m_Model->listVertex.push_back(glm::vec3(i, j, 0));
			m_Model->listCoords.push_back(glm::vec3((float)i/(float)m_nbElements, (float)j/(float)m_nbElements, 0));
			V.push_back(glm::vec3(0.0f));
			F.push_back(glm::vec3(0.0f));

		}
	//Resize and center

	glm::vec3 mean = glm::vec3(0.0f);
	glm::vec4 size = glm::vec4(0.0f);

	for (glm::vec3 v : m_Model->listVertex)
	{
		mean += v;
		size.x = std::min(v.x, size.x);
		size.y = std::max(v.x, size.y);

		size.z = std::min(v.y, size.z);
		size.w = std::max(v.y, size.w);
	}
	mean /= (float)m_Model->listVertex.size();
	for (int i = 0; i < (int)m_Model->listVertex.size(); i++)
	{
		m_Model->listVertex[i] = (m_Model->listVertex[i] - mean);
		m_Model->listVertex[i].x /= (size.y - size.x);
		m_Model->listVertex[i].y /= (size.w - size.z);
	}



	 m_Model->nb_vertex = (int)m_Model->listVertex.size();
	 m_Model->nb_faces = (int)m_Model->listFaces.size();
	 m_Model->loader->computeNormalAndTangents(m_Model);




	 /**************A COMPLETER *********************************************************
	 * Ajouter ici des ressorts entre les �l�ments de la grille 2D composant notre objet d�formable. Chaque ressort est une structure spring a rentrer dans le tableau springs.
	 * struct Spring
		{
			int id1;	// Indice du sommet 1
			int id2;	// Indice du sommet 2
			float length; // longueur au repos. D�finir la longueur initiale entre les 2 sommets par exemple
			float KsFactor; // facteur de rigidit�. // modulation du facteur de rigidit� global. Ce facteur va "moduler" le facteur de rigidit� globale pass� en param�tre. Utiliser 1.0 par exemple pour un ressort standard, 0.75 ou 0.5, ou moins pour un ressort plus fainle.
		};

		Il est conseill� de d�finir au moins un ressort standard entre chaque paire d'�l�ments de la grille en voisinage direct (haut,bas, gauche ,droite). 
		Il peut �tre �galement utile de d�finir des ressorts entre les �l�ments diagnouax , voir de voisinage plus eloign� (sommets a 2 el�ments de distance). Exp�rimentez diff�rentes strat�gies et mod�les afin d'obtenir un r�sultat convaincant.

	 */


	 /********************************************************************************************************************************************/



	 loadToGPU();
}

int CustomModelGL::indice(int i, int j)
{
	return i * m_nbElements + j;
}

void CustomModelGL::recomputeNormals()
{
	m_Model->listNormals.clear();
	m_Model->loader->computeNormals(m_Model);
}



void CustomModelGL::updatePositions()
{
	glNamedBufferData(VBO_Vertex, m_Model->nb_vertex * sizeof(glm::vec3), &(m_Model->listVertex.front()), GL_DYNAMIC_DRAW);
	glNamedBufferData(VBO_Normals, m_Model->nb_vertex * sizeof(glm::vec3), &(m_Model->listNormals.front()), GL_DYNAMIC_DRAW);
}