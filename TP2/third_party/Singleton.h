#ifndef _SINGLETON_GCLASS_
#define _SINGLETON_GCLASS_
#include <cstddef>
template <typename T>
class Singleton
{
	protected:
		// Constructeur/destructeur
		Singleton () { }
		~Singleton () { }

	public:
		// Interface publique
		static T *getInstance ()
		{
			if (_singleton == NULL)
				_singleton = new T;

			return (static_cast<T*> (_singleton));
		}

		static void kill ()
		{
			if (NULL != _singleton)
			{
				delete _singleton;
				_singleton = NULL;
			}
		}

	private:
		// Unique instance
		static T *_singleton;
};

template <typename T>
T *Singleton<T>::_singleton = NULL;


#endif
