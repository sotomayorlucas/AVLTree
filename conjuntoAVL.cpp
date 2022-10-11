#include <algorithm>
#include <iostream>
#include <random>
using namespace std;
#include <bits/stdc++.h>
#define COUNT 10
//Visto que la clave puede ser cualquier cosa podria no llamarlo conjunto sino AVL. Total un dicc es un pair donde no se repite el primer elemento.
//Creo que eso no funcionaria ahroa que lo pienso pq no quiero que se repitan claves, no claves y definicion.
//Tambien permitiria hacer varias definiciones de una misma clave ¿Podra mejorarse este codigo para eso o debo redefinirlos nodos obligatoriamente?
//Creo que podria agregar un elemento al nodo que sea T definicion, agregarlo al constructuor y cuando sea usado para conjuntos la definicion sea nullptr
// y que se agregue definicion cuando sea necesario (vease que debe hacerse una funcion que defina el nodo dado el nombre de la clave.

template <class T>
class NodoAVL {
public:
    T clave;
    int balanceo;
    NodoAVL *izquierda, *derecha, *padre;
    NodoAVL(T clave, NodoAVL *p) : clave(clave), balanceo(0), padre(p), izquierda(nullptr), derecha(nullptr) {}
    ~NodoAVL(){
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
    void borrar(T clave);
    const T& minimo() const;
    const T& maximo() const;
    void printBalance();
    void printAVL();

private: //Funciones necesarias para el funcionamiento del AVL pero no para el uso de conjuntos
    void destruir(NodoAVL<T>* raiz);
    NodoAVL<T>* _raiz;
    unsigned int _cardinal;
    NodoAVL<T>* rotacionIzquierda (NodoAVL<T>* a);
    NodoAVL<T>* rotacionDerecha (NodoAVL<T>* a);
    NodoAVL<T>* rotacionIzqLuegoDer (NodoAVL<T>* n);
    NodoAVL<T>* rotacionDerLuegoIzq (NodoAVL<T>* n);
    void rebalancear(NodoAVL<T>* n);
    int largo (NodoAVL<T>* n);
    void definirBalanceo(NodoAVL<T>* n);
    void printBalance ( NodoAVL<T> *n );
    void printAVL(NodoAVL<T>* root, int space);
};
//Defino funciones publicas

template <class T>
ConjuntoAVL<T>::ConjuntoAVL() {
    _raiz= nullptr;
    _cardinal=0;
}
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
ConjuntoAVL<T>::~ConjuntoAVL() {
    destruir(_raiz);
}

template <class T>
unsigned int ConjuntoAVL<T>::cardinal() const {
    return  _cardinal;
}
template <class T>
bool ConjuntoAVL<T>::pertenece(const T& clave) const {
    NodoAVL<T>* subArbolActual = _raiz;
    while (subArbolActual != nullptr && subArbolActual->clave != clave){
        if (clave < subArbolActual->clave){
            subArbolActual = subArbolActual->izquierda;
        } else if (subArbolActual->clave < clave){
            subArbolActual = subArbolActual->derecha;
        }
    }
    return subArbolActual != nullptr && subArbolActual->clave == clave;
}
template <class T>
void ConjuntoAVL<T>::insertar(const T& clave){
    if (cardinal() == 0){
        _raiz = new NodoAVL<T>(clave, nullptr);
        _cardinal++;
    }
        //Podria mejorarse para no evaluar en log n si pertenece o no la clave.
    else if (!pertenece(clave)){
        NodoAVL<T> *n = _raiz;
        NodoAVL<T> *padre;
        bool agregado = false;
        while (!agregado) {
            padre = n;
            bool irIzquierda = clave < n->clave;
            n = irIzquierda ? n->izquierda : n->derecha;
            if (n==nullptr){
                irIzquierda ? padre->izquierda = new NodoAVL<T>(clave,padre) : padre->derecha = new NodoAVL<T>(clave,padre);
                agregado = true;
                rebalancear(padre);
            }
        }
        _cardinal++;
    }
}

//Hago el borrado del Nodo, verifico si el arbol existe y luego busco el nodo. Si el elemento esta
//Entonces borro la clave. Si es la raiz pongo a su hijo como raiz, sino reubico los nodos con
//el nodo padre (creo que debo arreglar esto para que el hijo tenga al padre nuevo lol #creo la magia debe hacerse en rebalancear)
// y luego rebalanceo el arbol.
template <class T>
void ConjuntoAVL<T>::borrar(const T clave){
    if (_raiz == nullptr){
        return;
    }
    NodoAVL<T> *n = _raiz;
    NodoAVL<T> *padre = _raiz;
    NodoAVL<T> *nodBorrar = nullptr;
    NodoAVL<T> *hijo = _raiz;
    while(hijo != nullptr){
        padre = n;
        n = hijo;
        hijo = clave < n->clave ? n->izquierda : n->derecha;
        if (clave == n->clave) nodBorrar = n;
    }
    //Significa que el nodo a borrar esta en el conjunto/arbolAVL
    if (nodBorrar != nullptr){
        nodBorrar->clave = n->clave;
        hijo = n->izquierda != nullptr ? n->izquierda : n->derecha;
        //Veo si debo borrar la raiz
        if (_raiz->clave == clave) _raiz = hijo;
        else {
            if (padre->izquierda == n) padre->izquierda = hijo;
            else padre->derecha = hijo;
            rebalancear(padre);
        }
        _cardinal--;
    }
}

template <class T>
const T& ConjuntoAVL<T>::minimo() const {
    NodoAVL<T>* SubArbolActual = _raiz;
    while (SubArbolActual->izquierda != nullptr){
        SubArbolActual=SubArbolActual->izquierda;
    }
    return SubArbolActual->clave;
}

template <class T>
const T& ConjuntoAVL<T>::maximo() const {
    NodoAVL<T>* SubArbolActual = _raiz;
    while (SubArbolActual->derecha != nullptr){
        SubArbolActual=SubArbolActual->derecha;
    }
    return SubArbolActual->clave;
}

//Defino funciones privadas
template <class T>
void ConjuntoAVL<T>::rebalancear(NodoAVL<T>* n){
    definirBalanceo(n);
    if (n->balanceo == -2) {
        if (largo(n->izquierda->izquierda) >= largo(n->izquierda->derecha))
            n = rotacionDerecha(n);
            else n = rotacionIzqLuegoDer(n);
    }
    else if (n->balanceo == 2){
        if (largo(n->derecha->derecha) >= largo(n->derecha->izquierda))
            n = rotacionIzquierda(n);
        else n = rotacionDerLuegoIzq(n);
    }
    if (n->padre != nullptr)  rebalancear(n->padre); else  _raiz = n;
}
//TODO: Verificar si funciona con el Cormen o algo.
template <class T>
NodoAVL<T>* ConjuntoAVL<T>::rotacionIzquierda (NodoAVL<T>* a){
    NodoAVL<T> *b = a->derecha;
    b->padre= a->padre;
    a->derecha = b->izquierda;
    if (a->derecha != nullptr) a->derecha->padre = a;
    b->izquierda=a;
    a->padre=b;
    if(b->padre != nullptr){
        (b->padre->derecha == a) ? b->padre->derecha = b : b->padre->izquierda = b;
    }
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
    if(b->padre != nullptr){
        (b->padre->derecha == a) ? b->padre->derecha = b : b->padre->izquierda = b;
    }
    definirBalanceo(a);
    definirBalanceo(b);
    return b;
}

template <class T>
NodoAVL<T>* ConjuntoAVL<T>::rotacionIzqLuegoDer(NodoAVL<T> *n) {
    n->izquierda = rotacionIzquierda(n->izquierda);
    return rotacionDerecha(n);
}

template <class T>
NodoAVL<T>* ConjuntoAVL<T>::rotacionDerLuegoIzq(NodoAVL<T> *n) {
    n->derecha = rotacionDerecha(n->derecha);
    return rotacionIzquierda(n);
}

template <class T>
int ConjuntoAVL<T>::largo(NodoAVL<T>* n){
    int res;
    n == nullptr ? res = -1 : res = 1 + std::max(largo(n->izquierda), largo(n->derecha));
    return res;
}
template <class T>
void ConjuntoAVL<T>::definirBalanceo(NodoAVL<T> *n){
    n->balanceo = largo(n->derecha) - largo(n->izquierda);
}

//TODO:Averiguar como printear de forma comoda el arbol AVL del conjunto
/*
template<class T>
void ConjuntoAVL<T>::printAVL(const std::string& prefix, const NodoAVL<T>* node, bool isLeft)
{
    if( node != nullptr )
    {
        std::cout << prefix;

        std::cout << (isLeft ? "|--" : "L--" );

        // print the value of the node
        std::cout << node->clave << std::endl;

        // enter the next tree level - left and right branch
        printAVL( prefix + (isLeft ? "|   " : "    "), node->izquierda, true);
        printAVL( prefix + (isLeft ? "|   " : "    "), node->derecha, false);
    }
}
template<class T>
void ConjuntoAVL<T>::printAVL()
{
    printAVL("", _raiz, false);
}

// pass the root node of your binary tree
*/

template<class T>
void ConjuntoAVL<T>::printAVL(NodoAVL<T>* root, int space)
{
    // Base case
    if (root == nullptr)
        return;

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

// Wrapper over print2DUtil()
template<class T>
void ConjuntoAVL<T>::printAVL()
{
    // Pass initial space count as 0
    printAVL(_raiz, 0);
}


template <class T>
void ConjuntoAVL<T>::printBalance(NodoAVL<T> *n) {
    if (n != nullptr) {
        printBalance(n->izquierda);
        std::cout << n->balanceo << " ";
        printBalance(n->derecha);
    }
}

template <class T>
void ConjuntoAVL<T>::printBalance() {
    printBalance(_raiz);
    std::cout << std::endl;
}

int main(){
    ConjuntoAVL<int> c;
    vector<int> elementos;
    for (int i = 0; i < 10; ++i) {
        int n = rand();
        c.insertar(n);
        elementos.push_back(n);
    }

    for(int i = 0; i < 5; ++i){
        int random = rand() % elementos.size();
        int sel_elem = elementos[random];
        c.borrar(sel_elem);
    }
    std::cout << c.cardinal() << std::endl;
    c.printAVL();
}