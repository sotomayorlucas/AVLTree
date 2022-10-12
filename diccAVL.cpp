#include <algorithm>
#include <iostream>
using namespace std;
#define COUNT 10

//Adaptacion de conjuntoAVL para diccAVL.

//Los printsAVL los saque de una pagina que no se donde, pero no es mio :(
template <class T>
class NodoAVL {
public:
    T clave;
    T definicion;
    int balanceo;
    NodoAVL *izquierda, *derecha, *padre;
    NodoAVL(T clave, T definicion,NodoAVL *p) : clave(clave), definicion(definicion),  balanceo(0), padre(p), izquierda(nullptr), derecha(nullptr) {}
    ~NodoAVL(){
        izquierda = nullptr;
        derecha = nullptr;
        delete izquierda;
        delete derecha;
    }
};
template <class T>
class DiccionarioAVL {
public:
    DiccionarioAVL();
    ~DiccionarioAVL();
    unsigned int cardinal() const;
    bool definido(const T& clave) const;
    const T& obtener(const T& clave) const;
    void definir(const T& clave, const T& definicion);
    void borrar(const T& clave);
    const T& minimo() const;
    const T& maximo() const;
    void printAVL();

private: //Funciones necesarias para el funcionamiento del AVL pero no para el uso de diccionarios
    NodoAVL<T>* _raiz;
    unsigned int _cardinal;
    void destruir(NodoAVL<T>* raiz);
    void rebalancear(NodoAVL<T>* nodo);
    void definirBalanceo(NodoAVL<T>* nodo);
    NodoAVL<T>* rotacionIzquierda (NodoAVL<T>* nodo);
    NodoAVL<T>* rotacionDerecha (NodoAVL<T>* nodo);
    NodoAVL<T>* rotacionIzqLuegoDer (NodoAVL<T>* n);
    NodoAVL<T>* rotacionDerLuegoIzq (NodoAVL<T>* nodo);
    int largo (NodoAVL<T>* nodo);
    void removerHoja(NodoAVL<T> *nodoBorrar, NodoAVL<T> *padreNodo); //Sacados del taller del ABB
    void removerConUnHijo(NodoAVL<T>* nodoBorrar, NodoAVL<T> *padreNodo);
    void removerConDosHijos(NodoAVL<T> *nodoBorrar);
    NodoAVL<T>* maximoDeArbol(NodoAVL<T> *nodoRaiz);
    NodoAVL<T>* predecesorMaximo(NodoAVL<T> *nodo);
    void printAVL(NodoAVL<T>* root, int space);
};
/***************************************Defino funciones publicas******************************************/

template <class T>
DiccionarioAVL<T>::DiccionarioAVL() {
    _raiz= nullptr;
    _cardinal=0;
}

template <class T>
DiccionarioAVL<T>::~DiccionarioAVL() {
    destruir(_raiz);
}

template <class T>
unsigned int DiccionarioAVL<T>::cardinal() const {
    return  _cardinal;
}

template <class T>
bool DiccionarioAVL<T>::definido(const T& clave) const {
    NodoAVL<T>* nodo = _raiz;
    while (nodo != nullptr && nodo->clave != clave)
        (clave < nodo->clave) ? nodo = nodo->izquierda
                              : nodo = nodo->derecha;
    return nodo != nullptr && nodo->clave == clave;
}
//Pre:Debe estar definida la clave
template<class T>
const T& DiccionarioAVL<T>::obtener(const T &clave) const {
    NodoAVL<T>* nodo = _raiz;
    while (nodo != nullptr && nodo->clave != clave)
        (clave < nodo->clave) ? nodo = nodo->izquierda
                              : nodo = nodo->derecha;
    return nodo->definicion;
}

template <class T>
void DiccionarioAVL<T>::definir(const T& clave, const T& definicion){
    if (cardinal() == 0) _raiz = new NodoAVL<T>(clave,definicion,nullptr);
    else {
        NodoAVL<T> *nodo = _raiz;
        NodoAVL<T> *padre;
        bool agregado = false;
        while (!agregado) {
            padre = nodo;
            bool irIzquierda = clave < nodo->clave;
            if (nodo->clave == clave) {
                nodo->definicion = definicion;
                agregado = true;
            }
            nodo = irIzquierda ? nodo->izquierda : nodo->derecha;
            if (nodo==nullptr && !agregado){
                irIzquierda ? padre->izquierda = new NodoAVL<T>(clave,definicion,padre)
                            : padre->derecha = new NodoAVL<T>(clave,definicion,padre);
                agregado = true;
                rebalancear(padre);
            }
        }
    }
    _cardinal++;
}

//Hago el borrado del Nodo, verifico si el arbol existe y luego busco el nodo. Si el elemento esta
//Entonces borro la clave. Si es la raiz pongo a su hijo como raiz, sino reubico los nodos con
//el nodo padre (creo que debo arreglar esto para que el hijo tenga al padre nuevo lol #creo la magia debe hacerse en rebalancear)
// y luego rebalanceo el arbol.

template<class T>
void DiccionarioAVL<T>::borrar(const T& clave){
    if (_raiz == nullptr) return;
    NodoAVL<T> *nodo = _raiz;
    NodoAVL<T> *padre = nullptr;
    NodoAVL<T> *hijo = _raiz;
    while(hijo != nullptr && nodo->clave != clave){ //Itero hasta encontrar el nodo con la clave a borrar
        padre = nodo; nodo = hijo;
        hijo = clave < nodo->clave ? nodo->izquierda : nodo->derecha;
    }
    if (nodo->clave == clave ) {
        if (nodo->izquierda== nullptr && nodo -> derecha == nullptr)
            removerHoja(nodo, padre);
        else if (nodo->izquierda == nullptr || nodo->derecha == nullptr)
            removerConUnHijo(nodo,padre);
        else
            removerConDosHijos(nodo);
        _cardinal--;
        if (padre == nullptr && _raiz != nullptr) rebalancear(_raiz);
        else if (padre != nullptr) rebalancear(padre);
    }
}
template <class T>
const T& DiccionarioAVL<T>::minimo() const {
    NodoAVL<T>* nodo = _raiz;
    while (nodo->izquierda != nullptr) nodo=nodo->izquierda;
    return nodo->clave;
}

template <class T>
const T& DiccionarioAVL<T>::maximo() const {
    NodoAVL<T>* nodo = _raiz;
    while (nodo->derecha != nullptr) nodo=nodo->derecha;
    return nodo->clave;
}

template<class T>
void DiccionarioAVL<T>::printAVL()
{
    // Pass initial space count as 0
    printAVL(_raiz, 0);
}

/***************************************Defino funciones privadas******************************************/

template<class T>
void DiccionarioAVL<T>::destruir(NodoAVL<T> *raiz) {
    if (raiz != nullptr){
        destruir(raiz->izquierda);
        raiz->izquierda= nullptr;
        destruir(raiz->derecha);
        raiz->derecha= nullptr;
        delete raiz;
    }
}

template <class T>
void DiccionarioAVL<T>::rebalancear(NodoAVL<T>* nodo){
    definirBalanceo(nodo);
    if (nodo->balanceo == -2)
        (largo(nodo->izquierda->izquierda) >= largo(nodo->izquierda->derecha)) ?
                nodo = rotacionDerecha(nodo) : nodo = rotacionIzqLuegoDer(nodo);
    else if (nodo->balanceo == 2)
        (largo(nodo->derecha->derecha) >= largo(nodo->derecha->izquierda)) ?
                nodo = rotacionIzquierda(nodo) : nodo = rotacionDerLuegoIzq(nodo);
    if (nodo->padre != nullptr)  rebalancear(nodo->padre); else  _raiz = nodo;
}

template <class T>
void DiccionarioAVL<T>::definirBalanceo(NodoAVL<T> *nodo){
    nodo->balanceo = largo(nodo->derecha) - largo(nodo->izquierda);
}

template <class T>
NodoAVL<T>* DiccionarioAVL<T>::rotacionIzquierda (NodoAVL<T>* nodo){
    NodoAVL<T> *nuevoNodoRaiz = nodo->derecha; //Nuevo nodo raiz del subarbol que se enraizaba en el nodo.
    nuevoNodoRaiz->padre= nodo->padre;
    nodo->derecha = nuevoNodoRaiz->izquierda;
    if (nodo->derecha != nullptr) nodo->derecha->padre = nodo;
    nuevoNodoRaiz->izquierda=nodo;
    nodo->padre=nuevoNodoRaiz;
    if(nuevoNodoRaiz->padre != nullptr)
        (nuevoNodoRaiz->padre->derecha == nodo) ? nuevoNodoRaiz->padre->derecha = nuevoNodoRaiz
                                                : nuevoNodoRaiz->padre->izquierda = nuevoNodoRaiz;
    definirBalanceo(nodo);
    definirBalanceo(nuevoNodoRaiz);
    return nuevoNodoRaiz;
}
template<class T>
NodoAVL<T>* DiccionarioAVL<T>::rotacionDerecha(NodoAVL<T>* nodo){
    NodoAVL<T> *nuevoNodoRaiz = nodo->izquierda;
    nuevoNodoRaiz->padre= nodo->padre;
    nodo->izquierda = nuevoNodoRaiz->derecha;
    if (nodo->izquierda != nullptr) nodo->izquierda->padre = nodo;
    nuevoNodoRaiz->derecha=nodo;
    nodo->padre=nuevoNodoRaiz;
    if(nuevoNodoRaiz->padre != nullptr)
        (nuevoNodoRaiz->padre->derecha == nodo) ? nuevoNodoRaiz->padre->derecha = nuevoNodoRaiz
                                                : nuevoNodoRaiz->padre->izquierda = nuevoNodoRaiz;
    definirBalanceo(nodo);
    definirBalanceo(nuevoNodoRaiz);
    return nuevoNodoRaiz;
}

template <class T>
NodoAVL<T>* DiccionarioAVL<T>::rotacionIzqLuegoDer(NodoAVL<T> *nodo) {
    nodo->izquierda = rotacionIzquierda(nodo->izquierda);
    return rotacionDerecha(nodo);
}

template <class T>
NodoAVL<T>* DiccionarioAVL<T>::rotacionDerLuegoIzq(NodoAVL<T> *nodo) {
    nodo->derecha = rotacionDerecha(nodo->derecha);
    return rotacionIzquierda(nodo);
}

template <class T>
int DiccionarioAVL<T>::largo(NodoAVL<T>* nodo){
    return nodo == nullptr ? -1 : 1 + max(largo(nodo->izquierda), largo(nodo->derecha));
}

template<class T>
void DiccionarioAVL<T>::removerHoja(NodoAVL<T> *nodoBorrar, NodoAVL<T> *padreNodo) {
    if (padreNodo == nullptr) _raiz= nullptr;
    else padreNodo->derecha==nodoBorrar ? padreNodo->derecha= nullptr : padreNodo->izquierda= nullptr;
    delete nodoBorrar;
}
template <class T>
void DiccionarioAVL<T>::removerConUnHijo(NodoAVL<T>* nodoBorrar, NodoAVL<T> *padreNodo) {
    if (padreNodo == nullptr)//si es la raiz lo que quiero eliminar
        if (nodoBorrar->derecha== nullptr)  {
            _raiz=nodoBorrar->izquierda;
            _raiz->padre = nullptr;
        }
        else {
            _raiz= nodoBorrar->derecha;
            _raiz->padre = nullptr;
        }
    else {
        if (padreNodo->derecha == nodoBorrar) {
            nodoBorrar->izquierda == nullptr ? padreNodo->derecha = nodoBorrar->derecha
                                             : padreNodo->derecha = nodoBorrar->izquierda;
            padreNodo->derecha->padre = padreNodo;
        }
        else {
            nodoBorrar->izquierda == nullptr ? padreNodo->izquierda = nodoBorrar->derecha
                                             : padreNodo->izquierda = nodoBorrar->izquierda;
            padreNodo->izquierda->padre = padreNodo;
        }
    }
    delete nodoBorrar;
}

template <class T>
void DiccionarioAVL<T>::removerConDosHijos(NodoAVL<T> *nodoBorrar) {
    NodoAVL<T>* predMaximo = predecesorMaximo(nodoBorrar);
    NodoAVL<T>* padreDeMaximo = predMaximo->padre;
    nodoBorrar->clave=predMaximo->clave;
    if (predMaximo->izquierda== nullptr){ //me verifica si es hoja predMaximo
        if(padreDeMaximo->derecha == predMaximo){//verifica si el predMaximo es hijo derecho
            delete padreDeMaximo->derecha;
            padreDeMaximo->derecha = nullptr;
        }
        else {
            delete padreDeMaximo->izquierda;
            padreDeMaximo->izquierda = nullptr;
        }
    }
    else if (nodoBorrar->izquierda == predMaximo){
        nodoBorrar->izquierda=predMaximo->izquierda;
        delete predMaximo;
    }
    else {
        padreDeMaximo->derecha=predMaximo->izquierda;
        padreDeMaximo->derecha->padre = padreDeMaximo;
        delete predMaximo;
    }
}

template<class T>
NodoAVL<T>* DiccionarioAVL<T>::maximoDeArbol(NodoAVL<T> *nodoRaiz) {
    NodoAVL<T>* nodo = nodoRaiz;
    while (nodo->derecha != nullptr) nodo=nodo->derecha;
    return nodo;
}

template <class T>
NodoAVL<T>* DiccionarioAVL<T>::predecesorMaximo(NodoAVL<T> *nodo){
    NodoAVL<T>* predMaximo = maximoDeArbol(nodo->izquierda);
    return predMaximo;
}

template<class T>
void DiccionarioAVL<T>::printAVL(NodoAVL<T>* root, int space) {
    // Base case
    if (root == nullptr)  return;
    // Increase distance between levels
    space += COUNT;
    // Process right child first
    printAVL(root->derecha, space);
    // Print current node after space
    // count
    cout << endl;
    for (int i = COUNT; i < space; i++)
        cout << " ";
    cout << root->clave << ", "<< root->definicion << "\n";
    // Process left child
    printAVL(root->izquierda, space);
}

template <class T>
void correrPrograma (DiccionarioAVL<T> c) {
    int d;
    T n;
    T def;
    bool cerrarCiclo = false;
    while(!cerrarCiclo){
        cout << "Desea saber tamaño del diccionario (0) Ver si la clave esta definida y si si obtener definicion (1), agregar clave y definicion (2), borrar clave (3) maximo (4) minimo (5) cerrar(99) otra cosa printAVL" << endl;
        cin >> d;
        switch (d) {
            case 0:
                cout << "Tamaño del diccionario es " << c.cardinal() << endl; break;
            case 1: {
                cout << "N esta definido en el diccionario?" << endl; cin >> n; bool p = c.definido(n);
                p ? cout << "Clave " << n << " definida como "<< c.obtener(n) << endl : cout << n << " no esta definido" << endl;
                c.printAVL(); break;
            }
            case 2: {
                cout << "Ingrese la clave: " << endl; cin >> n;
                cout << "Ingrese la definicion: "<< endl; cin >> def;
                c.definir(n,def); c.printAVL(); break;
            }
            case 3:
                cout << "Ingrese el clave a borrar:" << endl; cin >> n; c.borrar(n); c.printAVL(); break;
            case 4:
                cout << "El maximo del diccionario es " << c.maximo() << endl; break;
            case 5:
                cout << "El minimo del diccionario es " << c.minimo() << endl; break;
            case 99:
                cerrarCiclo = true; break;
            default:
                c.printAVL(); break;
        }
    }
}

int main(){
    DiccionarioAVL<int> c;
    correrPrograma(c);
}