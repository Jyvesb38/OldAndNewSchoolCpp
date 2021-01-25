// ------------------------------------------------------------------------------------------------------
// Does the timing of standard operations such as creation, copy, and assignment on large objects,
// once with only copy operators defined (rule of three), then with move operators as well (rule of five)
// We may compare time execution on both cases.
// ------------------------------------------------------------------------------------------------------

#include <iostream>
#include <chrono>
#include <ctime>
#include <string>
#include <vector>

using namespace std;

// GENERAL :  les objets suivants sont RAII = "Resource Allocation Is Initialisation"
// resource allocation (or acquisition) is done during object creation (specifically initialization), by the constructor,
// while resource deallocation (release) is done during object destruction (specifically finalization), by the destructor.


// Chrono ===================================================
vector<std::chrono::system_clock::time_point> vStart;

inline void showChrono(string s) {
    auto cur = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = cur-vStart.back(); vStart.pop_back();
    std::cout << "current computation elapsed time : " << elapsed_seconds.count() << "s for " << s << "\n";
}
inline void setChrono0() { vStart.push_back(std::chrono::system_clock::now()); }
// ==========================================================

// ================================================================================
//                            OLD SCHOOL C++
// ================================================================================
class HolderOld
{
  public:

    HolderOld(int size)         // Constructor
    {
      m_data = new int[size];
      m_size = size;
    }

    HolderOld(const HolderOld& other)     // Copy constructor
    {
      m_data = new int[other.m_size];  // (1)
      std::copy(other.m_data, other.m_data + other.m_size, m_data);  // (2)
      m_size = other.m_size;
    }

    HolderOld& operator=(const HolderOld& other)    // Copy assignment operator
    {
      if(this == &other) return *this;  // (1)
      delete[] m_data;  // (2)
      m_data = new int[other.m_size];
      std::copy(other.m_data, other.m_data + other.m_size, m_data);
      m_size = other.m_size;
      return *this;  // (3)
    }

    ~HolderOld()                // Destructor
    {
      delete[] m_data;
    }

  private:

    int*   m_data;
    size_t m_size;
};

// ================================================================================
//                            NEW SCHOOL C++    (C++11)
// ================================================================================
class Holder
{
  public:

    Holder(int size)  noexcept       // Constructor
    {
      m_data = new int[size];
      m_size = size;
    }

    Holder(const Holder& other)     // Copy constructor
    {
      m_data = new int[other.m_size];  // (1)
      std::copy(other.m_data, other.m_data + other.m_size, m_data);  // (2)
      m_size = other.m_size;
    }

    Holder(Holder&& other) noexcept    // <-- rvalue reference in input
    {
      m_data = other.m_data;   // (1)           // Move constructor
      m_size = other.m_size;
      other.m_data = nullptr;  // (2)
      other.m_size = 0;
    }

    Holder& operator=(const Holder& other)      // Copy assignment operator
    {
      if(this == &other) return *this;  // (1)
      delete[] m_data;  // (2)
      m_data = new int[other.m_size];
      std::copy(other.m_data, other.m_data + other.m_size, m_data);
      m_size = other.m_size;
      return *this;  // (3)
    }

    Holder& operator=(Holder&& other) noexcept    // <-- rvalue reference in input
    {
      if (this == &other) return *this;         // Move assignment operator

      delete[] m_data;         // (1)

      m_data = other.m_data;   // (2)
      m_size = other.m_size;

      other.m_data = nullptr;  // (3)
      other.m_size = 0;

      return *this;
    }

    ~Holder()                // Destructor
    {
      delete[] m_data;
    }

  private:

    int*   m_data;
    size_t m_size;
};

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

int const arraysSize0 = 1073741824;
int const arraysSize1 = 536870912;
int const arraysSize2 = 268435456;
int const arraysSize3 = 134217728;

template<class holder>
holder createHolder(int size)
{
// On pourrait se contenter d'avoir la ligne telle quel :
//    return holder(size);
// Mais alors dans les 2 cas, "old school" et "new school",
// le compilateur optimise (Return Value Optimization RVO) et évite de faire un move constructor :
// - avec une copie de la data du tableau m_data (couteux) dans le cas old school avec une simple référence &
// - avec une simple affectation de l'@ m_data (rapide) dans le cas new school avec une forwarding reference &&
// Au lieu de ça, il fait un regular constructor directement dans la variable qui va recevoir le résultat de createHolder.
//
// Pour tromper le compilateur afin de passer par le move constructor dans tous les cas, on fait en sorte qu'il ne sache
// pas à l'avance quel sera le résultat à affecter : ci-dessous, h01 ou h02 ???
//
// De cette façon on peut effectivement mesurer la différence de perf du move constructor
// entre celui du old school (simple réf) et le new school (rvalue forward. ref)
//
// (On peut aussi inhiber le RVO pour GCC si on est sur Linux, avec le flag -fno-elide-constructors )
//

    time_t t = time(NULL);
    tm* timePtr = localtime(&t);

    holder
           h01(size),
           h02(size);

    if (timePtr->tm_mday % 2)   // selon jour du mois impair ou pair, retourne h01 ou h02
        return h01;
    else
        return h02;
}

template<class holder>
void process() {

    setChrono0();

    setChrono0();
    holder h1(arraysSize2);                // regular constructor
    showChrono("regular constructor");

    setChrono0();
    holder h2(h1);                  // copy constructor (lvalue in input)
    showChrono("copy constructor (lvalue in input)");

    setChrono0();
    holder h3(createHolder<holder>(arraysSize2)); // move constructor (rvalue in input) (1)
    showChrono("move constructor (rvalue in input)");

    setChrono0();
    h2 = h3;                        // assignment operator (lvalue in input)
    showChrono("assignment operator (lvalue in input)");

    setChrono0();
    h2 = createHolder<holder>(arraysSize1);         // move assignment operator (rvalue in input)
    showChrono("move assignment operator (rvalue in input)");

    showChrono("Total computation");

}


int main()
{

    cout << endl << endl << "-------------------------------------------------------" << endl;
    cout << "Old  School C++      C++98" << endl;
    process<HolderOld>();

    cout << endl << endl << "-------------------------------------------------------" << endl;
    cout << "New School C++       C++11" << endl;
    process<Holder>();

    //================================================================

    cout << endl << endl << "=======================================================" << endl;
    cout << "How to move a lvalue" << endl << endl;

    // std::move change une lvalue en rvalue

    setChrono0();
    {
        Holder h1(arraysSize0);     // h1 is an lvalue
        Holder h2(h1);              // copy-constructor invoked (because of lvalue in input)
    }
    showChrono("copy-constructor");

    setChrono0();
    {
        Holder h1(arraysSize0);     // h1 is an lvalue
        Holder h2(std::move(h1));   // move-constructor invoked (because of rvalue in input)

        // à ce stade, h1 est perdu, mais peu importe car on est à la fin du bloc de sa portée
    }
    showChrono("move-constructor");


    return 0;
}
