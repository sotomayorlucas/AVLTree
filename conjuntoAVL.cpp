#include <algorithm>
#include <iostream>
using namespace std;
#define COUNT 10
//Visto que la clave puede ser cualquier cosa podria no llamarlo conjunto sino AVL. Total un dicc es un pair donde no se repite el primer elemento.
//Creo que eso no funcionaria ahroa que lo pienso pq no quiero que se repitan claves, no claves y definicion.
//Tambien permitiria hacer varias definiciones de una misma clave ¿Podra mejorarse este codigo para eso o debo redefinirlos nodos obligatoriamente?
//Creo que podria agregar un elemento al nodo que sea T definicion, agregarlo al constructuor y cuando sea usado para conjuntos la definicion sea nullptr
// y que se agregue definicion cuando sea necesario (vease que debe hacerse una funcion que defina el nodo dado el nombre de la clave.


//Los printsAVL los saque de una pagina que no se donde, pero no es mio :(
template <class T>
class NodoAVL {
public:
    T clave;
    int balanceo;
    NodoAVL *izquierda, *derecha, *padre;
    NodoAVL(T clave, NodoAVL *p) : clave(clave), balanceo(0), padre(p), izquierda(nullptr), derecha(nullptr) {}
    ~NodoAVL(){
        izquierda = nullptr;
        derecha = nullptr;
        delete izquierda;
        delete derecha;
    }
};
template <class T>
class ConjuntoAVL {
public:
    ConjuntoAVL();
    ~ConjuntoAVL();
    unsigned int cardinal() const;
    bool pertenece(const T& clave) const;
    void insertar(const T& clave);
    void borrar(const T& clave);
    const T& minimo() const;
    const T& maximo() const;
    void printAVL();

private: //Funciones necesarias para el funcionamiento del AVL pero no para el uso de conjuntos
    NodoAVL<T>* _raiz;
    unsigned int _cardinal;
    void destruir(NodoAVL<T>* raiz);
    void rebalancear(NodoAVL<T>* nodo);
    void definirBalanceo(NodoAVL<T>* nodo);
    NodoAVL<T>* rotacionIzquierda (NodoAVL<T>* a);
    NodoAVL<T>* rotacionDerecha (NodoAVL<T>* a);
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
ConjuntoAVL<T>::ConjuntoAVL() {
    _raiz= nullptr;
    _cardinal=0;
}

template <class T>
ConjuntoAVL<T>::~ConjuntoAVL() {
    destruir(_raiz);
}

template <class T>
unsigned int ConjuntoAVL<T>::cardinal() const {
    return  _cardinal;
}

template <class T>
bool ConjuntoAVL<T>::pertenece(const T& clave) const {
    NodoAVL<T>* nodo = _raiz;
    while (nodo != nullptr && nodo->clave != clave)
        (clave < nodo->clave) ? nodo = nodo->izquierda : nodo = nodo->derecha;
    return nodo != nullptr && nodo->clave == clave;
}

template <class T>
void ConjuntoAVL<T>::insertar(const T& clave){
    if (cardinal() == 0) _raiz = new NodoAVL<T>(clave, nullptr);
    else {
        NodoAVL<T> *nodo = _raiz;
        NodoAVL<T> *padre;
        bool agregado = false;
        while (!agregado) {
            padre = nodo;
            bool irIzquierda = clave < nodo->clave;
            nodo->clave == clave ? agregado = true : agregado = false;
            nodo = irIzquierda ? nodo->izquierda : nodo->derecha;
            if (nodo==nullptr && !agregado){
                irIzquierda ? padre->izquierda = new NodoAVL<T>(clave,padre) : padre->derecha = new NodoAVL<T>(clave,padre);
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
void ConjuntoAVL<T>::borrar(const T& clave){
    if (_raiz == nullptr) return;
    NodoAVL<T> *nodo = _raiz;
    NodoAVL<T> *padre = _raiz;
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
        rebalancear(padre);
        _cardinal--;
    }
}
template <class T>
const T& ConjuntoAVL<T>::minimo() const {
    NodoAVL<T>* nodo = _raiz;
    while (nodo->izquierda != nullptr) nodo=nodo->izquierda;
    return nodo->clave;
}

template <class T>
const T& ConjuntoAVL<T>::maximo() const {
    NodoAVL<T>* nodo = _raiz;
    while (nodo->derecha != nullptr) nodo=nodo->derecha;
    return nodo->clave;
}

template<class T>
void ConjuntoAVL<T>::printAVL()
{
    // Pass initial space count as 0
    printAVL(_raiz, 0);
}

/***************************************Defino funciones privadas******************************************/

template<class T>
void ConjuntoAVL<T>::destruir(NodoAVL<T> *raiz) {
    if (raiz != nullptr){
        destruir(raiz->izquierda);
        raiz->izquierda= nullptr;
        destruir(raiz->derecha);
        raiz->derecha= nullptr;
        delete raiz;
    }
}

template <class T>
void ConjuntoAVL<T>::rebalancear(NodoAVL<T>* nodo){
    definirBalanceo(nodo);
    if (nodo->balanceo == -2) {
        if (largo(nodo->izquierda->izquierda) >= largo(nodo->izquierda->derecha))
            nodo = rotacionDerecha(nodo);
        else nodo = rotacionIzqLuegoDer(nodo);
    }
    else if (nodo->balanceo == 2){
        if (largo(nodo->derecha->derecha) >= largo(nodo->derecha->izquierda))
            nodo = rotacionIzquierda(nodo);
        else nodo = rotacionDerLuegoIzq(nodo);
    }
    if (nodo->padre != nullptr)  rebalancear(nodo->padre); else  _raiz = nodo;
}

template <class T>
void ConjuntoAVL<T>::definirBalanceo(NodoAVL<T> *nodo){
    nodo->balanceo = largo(nodo->derecha) - largo(nodo->izquierda);
}

template <class T>
NodoAVL<T>* ConjuntoAVL<T>::rotacionIzquierda (NodoAVL<T>* a){
    NodoAVL<T> *b = a->derecha;
    b->padre= a->padre;
    a->derecha = b->izquierda;
    if (a->derecha != nullptr) a->derecha->padre = a;
    b->izquierda=a;
    a->padre=b;
    if(b->padre != nullptr)
        (b->padre->derecha == a) ? b->padre->derecha = b : b->padre->izquierda = b;
    definirBalanceo(a);
    definirBalanceo(b);
    return b;
}
template<class T>
NodoAVL<T>* ConjuntoAVL<T>::rotacionDerecha(NodoAVL<T>* a){
    NodoAVL<T> *b = a->izquierda;
    b->padre= a->padre;
    a->izquierda = b->derecha;
    if (a->izquierda != nullptr) a->izquierda->padre = a;
    b->derecha=a;
    a->padre=b;
    if(b->padre != nullptr)
        (b->padre->derecha == a) ? b->padre->derecha = b : b->padre->izquierda = b;
    definirBalanceo(a);
    definirBalanceo(b);
    return b;
}

template <class T>
NodoAVL<T>* ConjuntoAVL<T>::rotacionIzqLuegoDer(NodoAVL<T> *nodo) {
    nodo->izquierda = rotacionIzquierda(nodo->izquierda);
    return rotacionDerecha(nodo);
}

template <class T>
NodoAVL<T>* ConjuntoAVL<T>::rotacionDerLuegoIzq(NodoAVL<T> *nodo) {
    nodo->derecha = rotacionDerecha(nodo->derecha);
    return rotacionIzquierda(nodo);
}

template <class T>
int ConjuntoAVL<T>::largo(NodoAVL<T>* n){
    return n == nullptr ? -1 : 1 + max(largo(n->izquierda), largo(n->derecha));
}

template<class T>
void ConjuntoAVL<T>::removerHoja(NodoAVL<T> *nodoBorrar, NodoAVL<T> *padreNodo) {
    if (padreNodo == nullptr) _raiz= nullptr;
    padreNodo->derecha==nodoBorrar ? padreNodo->derecha= nullptr : padreNodo->izquierda= nullptr;
    delete nodoBorrar;
}

template<class T>
void ConjuntoAVL<T>::removerConUnHijo(NodoAVL<T>* nodoBorrar, NodoAVL<T> *padreNodo) {
    if (padreNodo == nullptr)//si es la raiz lo que quiero eliminar
        (nodoBorrar->derecha== nullptr) ? _raiz=nodoBorrar->izquierda : _raiz= nodoBorrar->derecha;
    else {
        if (nodoBorrar->izquierda == nullptr) {
            if (padreNodo->derecha == nodoBorrar) {
                padreNodo->derecha = nodoBorrar->derecha;
                padreNodo->derecha->padre = padreNodo;
            } else {
                padreNodo->izquierda = nodoBorrar->derecha;
                padreNodo->izquierda->padre = padreNodo;
            }
        }
        else {
            if (padreNodo->derecha == nodoBorrar) {
                padreNodo->derecha = nodoBorrar->izquierda;
                padreNodo->derecha->padre = padreNodo;
            } else {
                padreNodo->izquierda = nodoBorrar->izquierda;
                padreNodo->izquierda->padre = padreNodo;
            }
        }
    }
    delete nodoBorrar;
}

template <class T>
void ConjuntoAVL<T>::removerConDosHijos(NodoAVL<T> *nodoBorrar) {
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
NodoAVL<T>* ConjuntoAVL<T>::maximoDeArbol(NodoAVL<T> *nodoRaiz) {
    NodoAVL<T>* nodo = nodoRaiz;
    while (nodo->derecha != nullptr) nodo=nodo->derecha;
    return nodo;
}

template <class T>
NodoAVL<T>* ConjuntoAVL<T>::predecesorMaximo(NodoAVL<T> *nodo){
    NodoAVL<T>* predMaximo = maximoDeArbol(nodo->izquierda);
    return predMaximo;
}

template<class T>
void ConjuntoAVL<T>::printAVL(NodoAVL<T>* root, int space) {
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
    cout << root->clave << "\n";
    // Process left child
    printAVL(root->izquierda, space);
}

template <class T>
void correrPrograma (ConjuntoAVL<T> c) {
    int d;
    int n;
    bool cerrarCiclo = false;
    while(!cerrarCiclo){
        cout << "Desea saber tamaño del conjunto (0) si n pertenece (1), agregar elemento (2), borrar elemento (3) maximo (4) minimo (5) cerrar(99) otra cosa printAVL" << endl;
        cin >> d;
        switch (d) {
            case 0:
                cout << "Tamaño del conjunto es " << c.cardinal() << endl; break;
            case 1: {
                cout << "N pertenece al conjunto?" << endl; cin >> n; bool p = c.pertenece(n);
                p ? cout << n << " pertenece" <<endl : cout << n << " no pertenece" << endl;
                c.printAVL(); break;
            }
            case 2: {
                cout << "Ingrese el elemento a agregar:" << endl; cin >> n; c.insertar(n); c.printAVL(); break;
            }
            case 3:
                cout << "Ingrese el elemento a borrar:" << endl; cin >> n; c.borrar(n); c.printAVL(); break;
            case 4:
                cout << "El maximo del conjunto es " << c.maximo() << endl; break;
            case 5:
                cout << "El minimo del conjunto es " << c.minimo() << endl; break;
            case 99:
                cerrarCiclo = true; break;
            default:
                c.printAVL(); break;
        }
    }
}

int main(){
    ConjuntoAVL<int> c;
    correrPrograma(c);
}