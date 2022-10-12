template<class T>
void ConjuntoAVL<T>::removerConUnHijo(NodoAVL<T>* nodoBorrar, NodoAVL<T> *padreNodo) {
    if (padreNodo == nullptr)//si es la raiz lo que quiero eliminar
        (nodoBorrar->derecha== nullptr) ? _raiz=nodoBorrar->izquierda : _raiz= nodoBorrar->derecha;
    else {
        if (nodoBorrar->izquierda == nullptr) {
            if (padreNodo->derecha == nodoBorrar) {
                padreNodo->derecha = nodoBorrar->derecha;
                padreNodo->derecha->padre = padreNodo;
            }
            else {
                padreNodo->izquierda = nodoBorrar->derecha;
                padreNodo->izquierda->padre = padreNodo;
            }
        }
        else {
            if (padreNodo->derecha == nodoBorrar) {
                padreNodo->derecha = nodoBorrar->izquierda;
                padreNodo->derecha->padre = padreNodo;
            }
            else {
                padreNodo->izquierda = nodoBorrar->izquierda;
                padreNodo->izquierda->padre = padreNodo;
            }
        }
    }
    delete nodoBorrar;
}