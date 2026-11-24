#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstdlib>
#include <windows.h>      // para los colores de la consola
#include <conio.h>        // para getch()

using namespace std;

// ---------- CONSTANTES ----------

const int AFORO = 100;                        // butacas de la sala
const char* ARCHIVO = "datos_cine.txt";       // archivo para guardar y cargar

const string REGISTRADO = "Registrado";       // estados posibles de un cliente
const string EN_FILA    = "En fila";
const string ATENDIDO   = "Atendido";

// ---------- NODOS DE LAS ESTRUCTURAS ----------

struct Cliente {              // nodo de la LISTA
    int id;
    string nombre;
    int prioridad;            // 1 (VIP) ... 10 (general)
    string estado;            // Registrado, En fila, Atendido
    Cliente* sig;
};

struct NodoFila {             // nodo de la COLA
    int id;                   // id del cliente
    int prioridad;
    NodoFila* sig;
};

struct NodoButaca {           // nodo de la PILA
    int idCliente;            // quien reservo
    int cantidad;             // cuantas butacas
    NodoButaca* sig;
};

// ---------- VARIABLES GLOBALES ----------

Cliente*    lista = NULL;     // inicio de la lista
NodoFila*   frente = NULL;    // frente de la cola
NodoButaca* cima = NULL;      // cima de la pila
int siguienteId = 1;          // el ID se asigna automaticamente
int butacasUsadas = 0;        // total de butacas reservadas

// Prototipos: estas funciones se definen mas abajo pero la lista las usa antes
bool estaEnFila(int id);
bool quitarDeFila(int id);
void encolar(int id, int prioridad);
int butacasDeCliente(int id);

// =========================================================
// COLORES Y MENSAJES
// =========================================================

// Cambia el color del texto de la consola
void color(int c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void limpiar() {
    system("cls");
}

void pausa() {
    color(8);
    cout << "\nPresione una tecla para continuar...";
    color(7);
    getch();
}

void titulo(string texto) {
    color(11);
    cout << "\n==================================================\n";
    color(14);
    cout << "   " << texto << "\n";
    color(11);
    cout << "==================================================\n";
    color(7);
}

void mensajeOk(string m) {
    color(10);
    cout << "\n[ OK ] " << m << "\n";
    color(7);
}

void mensajeError(string m) {
    color(12);
    cout << "\n[ ERROR ] " << m << "\n";
    color(7);
}

// =========================================================
// LECTURA CON VALIDACION
// =========================================================

// Pide un numero entre minimo y maximo. Repite hasta que sea valido.
// Rechaza letras ("abc"), mezclas ("5abc") y numeros fuera del rango.
int leerEntero(string mensaje, int minimo, int maximo) {
    int n;
    while (true) {
        color(14);
        cout << mensaje;
        color(15);
        cin >> n;
        color(7);
        if (cin.eof()) exit(0);                   // se cerro la entrada

        // valido: se leyo un numero, la linea termina ahi y esta en rango
        if (!cin.fail() && cin.peek() == '\n' && n >= minimo && n <= maximo) {
            cin.ignore(1000, '\n');
            return n;
        }
        cin.clear();                              // limpia el error
        cin.ignore(1000, '\n');                   // descarta lo escrito
        color(12);
        cout << "Dato invalido. Ingrese un numero de " << minimo << " a " << maximo << ".\n";
    }
}

// Pide un texto de 1 a 20 caracteres que no sea solo espacios
string leerTexto(string mensaje) {
    string t;
    while (true) {
        color(14);
        cout << mensaje;
        color(15);
        getline(cin, t);
        color(7);
        if (cin.eof()) exit(0);                   // se cerro la entrada

        if (t.size() > 0 && t.size() <= 20 && t.find_first_not_of(' ') != string::npos)
            return t;
        color(12);
        cout << "Escriba un nombre de 1 a 20 caracteres.\n";
    }
}

// =========================================================
// 1. LISTA ENLAZADA - REGISTRO DE CLIENTES
// =========================================================

// Buscar por ID: recorre la lista nodo por nodo
Cliente* buscarPorId(int id) {
    Cliente* actual = lista;
    while (actual != NULL) {
        if (actual->id == id) return actual;
        actual = actual->sig;
    }
    return NULL;
}

// Buscar por nombre (exacto)
Cliente* buscarPorNombre(string nombre) {
    Cliente* actual = lista;
    while (actual != NULL) {
        if (actual->nombre == nombre) return actual;
        actual = actual->sig;
    }
    return NULL;
}

// Insertar: agrega un nodo al final de la lista
void insertarFinal(int id, string nombre, int prioridad, string estado) {
    Cliente* nuevo = new Cliente;
    nuevo->id = id;
    nuevo->nombre = nombre;
    nuevo->prioridad = prioridad;
    nuevo->estado = estado;
    nuevo->sig = NULL;

    if (lista == NULL) {
        lista = nuevo;                       // lista vacia: es el primero
    } else {
        Cliente* ultimo = lista;
        while (ultimo->sig != NULL) ultimo = ultimo->sig;
        ultimo->sig = nuevo;
    }
}

// Eliminar: busca el nodo y lo desconecta de la lista
bool eliminarCliente(int id) {
    Cliente* actual = lista;
    Cliente* anterior = NULL;

    while (actual != NULL && actual->id != id) {
        anterior = actual;
        actual = actual->sig;
    }
    if (actual == NULL) return false;              // no existe

    if (anterior == NULL) lista = actual->sig;     // era el primero
    else anterior->sig = actual->sig;

    quitarDeFila(id);                              // si estaba en la fila, tambien sale
    delete actual;
    return true;
}

// Modificar prioridad (si esta en la fila, se vuelve a ubicar)
void modificarPrioridad(Cliente* c, int nueva) {
    if (c->prioridad == nueva) return;             // no cambia nada: conserva su turno
    c->prioridad = nueva;
    if (estaEnFila(c->id)) {
        quitarDeFila(c->id);
        encolar(c->id, nueva);
    }
}

void mostrarClientes() {
    if (lista == NULL) {
        mensajeError("No hay clientes registrados.");
        return;
    }
    color(11);
    cout << "\n+------+----------------------+-----------+------------+\n";
    cout << "| ID   | NOMBRE               | PRIORIDAD | ESTADO     |\n";
    cout << "+------+----------------------+-----------+------------+\n";
    color(15);
    for (Cliente* c = lista; c != NULL; c = c->sig) {
        cout << "| " << setw(4) << left << c->id
             << " | " << setw(20) << left << c->nombre
             << " | " << setw(9) << right << c->prioridad
             << " | " << setw(10) << left << c->estado << " |\n";
    }
    color(11);
    cout << "+------+----------------------+-----------+------------+\n";
    color(7);
}

// =========================================================
// 2. COLA - FILA DE ENTRADA (POR PRIORIDAD)
// =========================================================

// Buscar: devuelve la posicion en la fila (0 si no esta)
int posicionEnFila(int id) {
    int pos = 1;
    for (NodoFila* a = frente; a != NULL; a = a->sig) {
        if (a->id == id) return pos;
        pos++;
    }
    return 0;
}

bool estaEnFila(int id) {
    return posicionEnFila(id) > 0;
}

// Encolar: inserta segun prioridad (numero menor = pasa primero).
// Con la misma prioridad respeta el orden de llegada.
void encolar(int id, int prioridad) {
    NodoFila* nuevo = new NodoFila;
    nuevo->id = id;
    nuevo->prioridad = prioridad;
    nuevo->sig = NULL;

    if (frente == NULL || prioridad < frente->prioridad) {
        nuevo->sig = frente;                 // va al frente
        frente = nuevo;
    } else {
        NodoFila* actual = frente;
        while (actual->sig != NULL && actual->sig->prioridad <= prioridad)
            actual = actual->sig;
        nuevo->sig = actual->sig;            // va en medio o al final
        actual->sig = nuevo;
    }
}

// Desencolar: saca el del frente. Devuelve su ID o -1 si esta vacia
int desencolar() {
    if (frente == NULL) return -1;
    NodoFila* aux = frente;
    int id = aux->id;
    frente = frente->sig;
    delete aux;
    return id;
}

// Quitar un cliente especifico de la fila
bool quitarDeFila(int id) {
    NodoFila* actual = frente;
    NodoFila* anterior = NULL;
    while (actual != NULL && actual->id != id) {
        anterior = actual;
        actual = actual->sig;
    }
    if (actual == NULL) return false;
    if (anterior == NULL) frente = actual->sig;
    else anterior->sig = actual->sig;
    delete actual;
    return true;
}

void mostrarFila() {
    if (frente == NULL) {
        mensajeError("La fila de entrada esta vacia.");
        return;
    }
    color(11);
    cout << "\n+-----+------+----------------------+-----------+\n";
    cout << "| POS | ID   | NOMBRE               | PRIORIDAD |\n";
    cout << "+-----+------+----------------------+-----------+\n";
    int pos = 1;
    for (NodoFila* f = frente; f != NULL; f = f->sig) {
        Cliente* c = buscarPorId(f->id);
        color(pos == 1 ? 10 : 15);               // el primero se ve en verde
        cout << "| " << setw(3) << right << pos
             << " | " << setw(4) << left << f->id
             << " | " << setw(20) << left << c->nombre
             << " | " << setw(9) << right << f->prioridad << " |\n";
        pos++;
    }
    color(11);
    cout << "+-----+------+----------------------+-----------+\n";
    color(10);
    cout << "El de la posicion 1 es el siguiente en ser atendido.\n";
    color(7);
}

// =========================================================
// 3. PILA - BUTACAS RESERVADAS
// =========================================================

// Push: apila una reserva (un bloque de butacas)
void push(int idCliente, int cantidad) {
    NodoButaca* nuevo = new NodoButaca;
    nuevo->idCliente = idCliente;
    nuevo->cantidad = cantidad;
    nuevo->sig = cima;
    cima = nuevo;
    butacasUsadas += cantidad;
}

// Pop: cancela la reserva de la cima y devuelve su cantidad (0 si la pila esta vacia)
int pop() {
    if (cima == NULL) return 0;
    NodoButaca* aux = cima;
    int cantidad = aux->cantidad;
    butacasUsadas = butacasUsadas - cantidad;
    cima = cima->sig;
    delete aux;
    return cantidad;
}

// Buscar: cuantas butacas tiene reservadas un cliente
int butacasDeCliente(int id) {
    int total = 0;
    for (NodoButaca* a = cima; a != NULL; a = a->sig)
        if (a->idCliente == id) total += a->cantidad;
    return total;
}

// Muestra el estado de la sala y las reservas (de la cima a la base)
void mostrarSala() {
    color(11);
    cout << "\nAforo de la sala:   " << AFORO << " butacas\n";
    cout << "Butacas reservadas: " << butacasUsadas << "\n";
    cout << "Butacas libres:     " << AFORO - butacasUsadas << "\n";

    if (cima == NULL) {
        mensajeError("No hay reservas registradas.");
        return;
    }
    color(11);
    cout << "\n+------+----------------------+----------+\n";
    cout << "| ID   | CLIENTE              | CANTIDAD |\n";
    cout << "+------+----------------------+----------+\n";
    for (NodoButaca* b = cima; b != NULL; b = b->sig) {
        Cliente* c = buscarPorId(b->idCliente);
        color(b == cima ? 10 : 15);              // la cima se ve en verde
        cout << "| " << setw(4) << left << b->idCliente
             << " | " << setw(20) << left << c->nombre
             << " | " << setw(8) << right << b->cantidad << " |"
             << (b == cima ? " <- CIMA" : "") << "\n";
    }
    color(11);
    cout << "+------+----------------------+----------+\n";
    color(7);
}

// =========================================================
// GUARDAR Y CARGAR (persistencia)
// =========================================================

void guardarDatos() {
    ofstream f(ARCHIVO);
    if (!f) {
        mensajeError("No se pudo crear el archivo.");
        return;
    }
    int cantidad = 0;

    f << siguienteId << "\n";

    // clientes: id, nombre, prioridad, estado (uno por linea)
    for (Cliente* c = lista; c != NULL; c = c->sig) cantidad++;
    f << cantidad << "\n";
    for (Cliente* c = lista; c != NULL; c = c->sig)
        f << c->id << "\n" << c->nombre << "\n" << c->prioridad << "\n" << c->estado << "\n";

    // fila (solo los id, en orden)
    cantidad = 0;
    for (NodoFila* n = frente; n != NULL; n = n->sig) cantidad++;
    f << cantidad << "\n";
    for (NodoFila* n = frente; n != NULL; n = n->sig) f << n->id << "\n";

    // pila (de la cima a la base)
    cantidad = 0;
    for (NodoButaca* b = cima; b != NULL; b = b->sig) cantidad++;
    f << cantidad << "\n";
    for (NodoButaca* b = cima; b != NULL; b = b->sig)
        f << b->idCliente << "\n" << b->cantidad << "\n";

    f.close();
    mensajeOk("Datos guardados en " + string(ARCHIVO));
}

// Borra todo lo que haya en memoria (libera los nodos)
void liberarTodo() {
    while (lista != NULL) {
        Cliente* aux = lista;
        lista = lista->sig;
        delete aux;
    }
    while (frente != NULL) {
        NodoFila* aux = frente;
        frente = frente->sig;
        delete aux;
    }
    while (cima != NULL) {
        NodoButaca* aux = cima;
        cima = cima->sig;
        delete aux;
    }
    butacasUsadas = 0;
    siguienteId = 1;
}

// Lee una linea del archivo y la convierte a numero
int leerLineaEntero(ifstream &f) {
    string s;
    getline(f, s);
    return atoi(s.c_str());
}

bool cargarDatos() {
    ifstream f(ARCHIVO);
    if (!f) return false;                         // no existe el archivo

    int id = leerLineaEntero(f);
    if (id < 1) return false;                     // archivo vacio o danado: no se toca nada

    liberarTodo();
    siguienteId = id;
    int n, i;

    n = leerLineaEntero(f);                       // clientes
    for (i = 0; i < n; i++) {
        int idCliente = leerLineaEntero(f);
        string nombre, estado;
        getline(f, nombre);
        int prio = leerLineaEntero(f);
        getline(f, estado);
        insertarFinal(idCliente, nombre, prio, estado);
    }

    n = leerLineaEntero(f);                       // fila
    for (i = 0; i < n; i++) {
        Cliente* c = buscarPorId(leerLineaEntero(f));
        if (c != NULL) encolar(c->id, c->prioridad);
    }

    n = leerLineaEntero(f);                       // pila
    if (n > AFORO) n = AFORO;                     // nunca hay mas reservas que butacas
    int ids[AFORO], cants[AFORO];                 // se guardo de la cima a la base
    for (i = 0; i < n; i++) {
        ids[i] = leerLineaEntero(f);
        cants[i] = leerLineaEntero(f);
    }
    for (i = n - 1; i >= 0; i--)                  // se apila empezando por la base
        push(ids[i], cants[i]);

    f.close();
    return true;
}

// =========================================================
// OPCIONES DEL MENU
// =========================================================

void opRegistrar() {
    limpiar();
    titulo("REGISTRAR CLIENTE");
    string nombre = leerTexto("Nombre del cliente: ");
    if (buscarPorNombre(nombre) != NULL) {
        mensajeError("Ya existe un cliente con ese nombre.");
        return;
    }
    int prio = leerEntero("Prioridad (1 = VIP ... 10 = general): ", 1, 10);
    insertarFinal(siguienteId, nombre, prio, REGISTRADO);
    mensajeOk("Cliente registrado.");
    color(15);
    cout << "ID asignado: " << siguienteId << "\n";
    color(7);
    siguienteId++;
}

void opEliminar() {
    limpiar();
    titulo("ELIMINAR CLIENTE");
    mostrarClientes();
    if (lista == NULL) return;
    int id = leerEntero("\nID a eliminar: ", 1, 9999);
    if (buscarPorId(id) == NULL)
        mensajeError("No existe ese cliente.");
    else if (butacasDeCliente(id) > 0)
        mensajeError("Tiene butacas reservadas. Cancele sus reservas primero.");
    else {
        eliminarCliente(id);
        mensajeOk("Cliente eliminado.");
    }
}

void opBuscar() {
    limpiar();
    titulo("BUSCAR CLIENTE");
    color(11);
    cout << "1. Por ID\n2. Por nombre\n";
    color(7);
    int tipo = leerEntero("Opcion: ", 1, 2);
    Cliente* c;
    if (tipo == 1) c = buscarPorId(leerEntero("ID: ", 1, 9999));
    else c = buscarPorNombre(leerTexto("Nombre exacto: "));

    if (c == NULL) {
        mensajeError("Cliente no encontrado.");
        return;
    }
    color(15);
    cout << "\nID:        " << c->id;
    cout << "\nNombre:    " << c->nombre;
    cout << "\nPrioridad: " << c->prioridad;
    cout << "\nEstado:    " << c->estado;
    if (estaEnFila(c->id)) cout << "\nPosicion en la fila: " << posicionEnFila(c->id);
    cout << "\nButacas reservadas: " << butacasDeCliente(c->id) << "\n";
    color(7);
}

void opModificarPrioridad() {
    limpiar();
    titulo("CAMBIAR PRIORIDAD");
    mostrarClientes();
    if (lista == NULL) return;
    int id = leerEntero("\nID del cliente: ", 1, 9999);
    Cliente* c = buscarPorId(id);
    if (c == NULL) {
        mensajeError("No existe ese cliente.");
    } else if (c->estado == ATENDIDO) {
        mensajeError("Ese cliente ya fue atendido.");
    } else {
        modificarPrioridad(c, leerEntero("Nueva prioridad (1-10): ", 1, 10));
        mensajeOk("Prioridad modificada.");
    }
}

// Cambio de estado: Registrado -> En fila
void opMeterEnFila() {
    limpiar();
    titulo("METER CLIENTE EN LA FILA");
    mostrarClientes();
    if (lista == NULL) return;
    int id = leerEntero("\nID del cliente: ", 1, 9999);
    Cliente* c = buscarPorId(id);
    if (c == NULL) {
        mensajeError("No existe ese cliente.");
    } else if (c->estado == ATENDIDO) {
        mensajeError("Ese cliente ya fue atendido.");
    } else if (estaEnFila(id)) {
        mensajeError("Ese cliente ya esta en la fila.");
    } else {
        c->estado = EN_FILA;
        encolar(c->id, c->prioridad);
        mensajeOk("Cliente en la fila.");
    }
}

// Cambio de estado: En fila -> Atendido
void opAtender() {
    limpiar();
    titulo("ATENDER SIGUIENTE CLIENTE");
    int id = desencolar();
    if (id == -1) {
        mensajeError("La fila esta vacia, no hay nadie que atender.");
        return;
    }
    Cliente* c = buscarPorId(id);
    c->estado = ATENDIDO;
    color(10);
    cout << "\nAtendiendo a: " << c->nombre << " (prioridad " << c->prioridad << ")\n";
    mensajeOk("Cliente atendido. Ya puede reservar butacas.");
}

void opReservar() {
    limpiar();
    titulo("RESERVAR BUTACAS (PUSH)");
    mostrarClientes();
    if (lista == NULL) return;
    cout << "\nButacas libres: " << AFORO - butacasUsadas << "\n";
    int id = leerEntero("ID del cliente: ", 1, 9999);
    Cliente* c = buscarPorId(id);
    if (c == NULL) {
        mensajeError("No existe ese cliente.");
    } else if (c->estado != ATENDIDO) {
        mensajeError("El cliente aun no fue atendido. Debe pasar por la fila.");
    } else {
        int cant = leerEntero("Cantidad de butacas: ", 1, AFORO);
        if (butacasUsadas + cant > AFORO) {
            mensajeError("No hay butacas libres suficientes.");
        } else {
            push(id, cant);
            mensajeOk("Butacas reservadas.");
        }
    }
}

void opCancelarReserva() {
    limpiar();
    titulo("CANCELAR ULTIMA RESERVA (POP)");
    if (cima == NULL) {
        mensajeError("No hay reservas para cancelar (pila vacia).");
        return;
    }
    Cliente* c = buscarPorId(cima->idCliente);   // antes de sacar la reserva
    string nombre = c->nombre;
    int cant = pop();
    mensajeOk("Reserva cancelada.");
    color(15);
    cout << "Se liberaron " << cant << " butacas de " << nombre << ".\n";
    color(7);
}

// Cargar datos: avisa que se pierden los cambios sin guardar
void opCargar() {
    limpiar();
    titulo("CARGAR DATOS");
    color(14);
    cout << "Se reemplazara todo lo actual por lo guardado en el archivo.\n";
    color(7);
    if (leerEntero("Desea continuar? (1 = Si, 0 = No): ", 0, 1) == 0) {
        mensajeError("Operacion cancelada. No se cargo nada.");
        return;
    }
    if (cargarDatos()) mensajeOk("Datos cargados.");
    else mensajeError("No hay archivo o esta danado. No se modifico nada.");
}

// =========================================================
// MENU DE COLORES
// =========================================================

// Dibuja UNA linea de un cuadro de opcion (linea 1 = texto, 0 = borde)
void lineaCuadro(int col, string texto, int linea) {
    color(col);
    if (texto == "")
        cout << "                         ";          // sin cuadro (espacio vacio)
    else if (linea == 1)
        cout << "| " << setw(21) << left << texto << " |";
    else
        cout << "+-----------------------+";
}

// Dibuja una fila de hasta 3 cuadros, cada uno con su color
void filaMenu(int c1, string t1, int c2, string t2, int c3, string t3) {
    for (int linea = 0; linea < 3; linea++) {
        int tipo = (linea == 1) ? 1 : 0;
        cout << " ";
        lineaCuadro(c1, t1, tipo);
        cout << " ";
        lineaCuadro(c2, t2, tipo);
        cout << " ";
        lineaCuadro(c3, t3, tipo);
        cout << "\n";
    }
}

int menu() {
    limpiar();
    color(11);
    cout << "==============================================================================\n";
    color(14);
    cout << "                  SISTEMA DE GESTION DE PROCESOS DE UN CINE\n";
    color(11);
    cout << "==============================================================================\n";

    filaMenu(10, "1. Registrar cliente",  12, "2. Eliminar cliente",   11, "3. Buscar cliente");
    filaMenu(14, "4. Cambiar prioridad",   9, "5. Mostrar clientes",   13, "6. Meter en la fila");
    filaMenu(10, "7. Atender siguiente",  11, "8. Mostrar fila",       14, "9. Reservar butacas");
    filaMenu(12, "10. Cancelar reserva",   9, "11. Estado de la sala", 13, "12. Guardar datos");
    filaMenu(10, "13. Cargar datos",      12, "0. SALIR",               7, "");

    color(11);
    cout << "==============================================================================\n";
    return leerEntero("                         Seleccione una opcion: ", 0, 13);
}

// =========================================================
// PROGRAMA PRINCIPAL
// =========================================================

int main() {
    // si hay datos guardados, se cargan al iniciar
    if (!cargarDatos()) {
        // (no hay archivo o esta danado: se inicia vacio)
    }

    int opcion;
    do {
        opcion = menu();
        switch (opcion) {
            case 1:  opRegistrar();          break;
            case 2:  opEliminar();           break;
            case 3:  opBuscar();             break;
            case 4:  opModificarPrioridad(); break;
            case 5:  limpiar(); titulo("LISTA DE CLIENTES"); mostrarClientes(); break;
            case 6:  opMeterEnFila();        break;
            case 7:  opAtender();            break;
            case 8:  limpiar(); titulo("FILA DE ENTRADA"); mostrarFila(); break;
            case 9:  opReservar();           break;
            case 10: opCancelarReserva();    break;
            case 11: limpiar(); titulo("ESTADO DE LA SALA"); mostrarSala(); break;
            case 12: limpiar(); titulo("GUARDAR DATOS"); guardarDatos(); break;
            case 13: opCargar();             break;
            case 0:
                limpiar();
                titulo("SALIR");
                if (leerEntero("Guardar los datos antes de salir? (1 = Si, 0 = No): ", 0, 1) == 1)
                    guardarDatos();
                color(10);
                cout << "\nGracias por usar el sistema. Hasta luego!\n\n";
                color(7);
                break;
        }
        if (opcion != 0) pausa();
    } while (opcion != 0);

    liberarTodo();
    return 0;
}