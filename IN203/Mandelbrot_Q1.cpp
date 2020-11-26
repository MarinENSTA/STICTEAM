# include <iostream>
# include <cstdlib>
# include <string>
# include <chrono>
# include <cmath>
# include <vector>
# include <fstream>
# include <mpi.h>

//156 = valeur séquentielle de ref


/** Une structure complexe est définie pour la bonne raison que la classe
 * complex proposée par g++ est très lente ! Le calcul est bien plus rapide
 * avec la petite structure donnée ci--dessous
 **/
struct Complex
{
    Complex() : real(0.), imag(0.)
    {}
    Complex(double r, double i) : real(r), imag(i)
    {}
    Complex operator + ( const Complex& z )
    {
        return Complex(real + z.real, imag + z.imag );
    }
    Complex operator * ( const Complex& z )
    {
        return Complex(real*z.real-imag*z.imag, real*z.imag+imag*z.real);
    }
    double sqNorm() { return real*real + imag*imag; }
    double real,imag;
};

std::ostream& operator << ( std::ostream& out, const Complex& c )
{
  out << "(" << c.real << "," << c.imag << ")" << std::endl;
  return out;
}


int iterMandelbrot( int maxIter, const Complex& c)
{
    Complex z{0.,0.};
    // On vérifie dans un premier temps si le complexe
    // n'appartient pas à une zone de convergence connue :
    // Appartenance aux disques  C0{(0,0),1/4} et C1{(-1,0),1/4}
    if ( c.real*c.real+c.imag*c.imag < 0.0625 )
        return maxIter;
    if ( (c.real+1)*(c.real+1)+c.imag*c.imag < 0.0625 )
        return maxIter;
    // Appartenance à la cardioïde {(1/4,0),1/2(1-cos(theta))}    
    if ((c.real > -0.75) && (c.real < 0.5) ) {
        Complex ct{c.real-0.25,c.imag};
        double ctnrm2 = sqrt(ct.sqNorm());
        if (ctnrm2 < 0.5*(1-ct.real/ctnrm2)) return maxIter;
    }
    int niter = 0;
    while ((z.sqNorm() < 4.) && (niter < maxIter))
    {
        z = z*z + c;
        ++niter;
    }
    return niter;
}


void 
computeMandelbrotSetRow( int W, int H, int maxIter, int num_ligne, int* pixels)
{
    // Calcul le facteur d'échelle pour rester dans le disque de rayon 2
    // centré en (0,0)
    double scaleX = 3./(W-1);
    double scaleY = 2.25/(H-1.);
    //
    // On parcourt les pixels de l'espace image :
    // for ( int j = 0; j < W; ++j ) {
    //    Complex c{-2.+j*scaleX,-1.125+ num_ligne*scaleY};
    //    pixels[j] = iterMandelbrot( maxIter, c );
    // }
    for ( int j = 0; j < W; ++j ) {
        Complex c{-2.+j*scaleX,-1.125+ num_ligne*scaleY};
        pixels[j] = iterMandelbrot( maxIter, c );
    }

}

std::vector<int>
computeMandelbrotSet( int W, int H, int maxIter )
{
std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    int nbproc;
    MPI_Comm_size(MPI_COMM_WORLD, &nbproc);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm globComm;
    MPI_Comm_dup(MPI_COMM_WORLD, &globComm);

    if (H%nbproc !=0 ) { std::cerr << "H n'est pas divisible par nbproc" << std::endl ; MPI_Abort(globComm,EXIT_FAILURE) ; }
    

    int imin_loc = H / nbproc * (nbproc-rank-1) ;
    int imax_loc = H / nbproc * (nbproc-rank) ;
    int H_loc = H / nbproc ;
    std::vector<int> pixels(W*H);
    
    std::vector<int> pixels_loc(W*H_loc);

    for ( int i = imin_loc; i < imax_loc; ++i ) {
      computeMandelbrotSetRow(W, H, maxIter, i, pixels_loc.data() + W*(imax_loc-i-1) );
    }

    // MPI_Gather(pixels, pixels_loc, ...); 

    MPI_Gather(&pixels_loc[0], pixels_loc.size(), MPI_INT, &pixels[0], W*H_loc, MPI_INT, 0, globComm);


    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "Temps calcul ensemble mandelbrot sur process ["<<rank<<"] :" << elapsed_seconds.count() 
              << std::endl;

    return pixels;
}


/** Construit et sauvegarde l'image finale **/
void savePicture( const std::string& filename, int W, int H, const std::vector<int>& nbIters, int maxIter )
{
    double scaleCol = 1./maxIter;//16777216
    std::ofstream ofs( filename.c_str(), std::ios::out | std::ios::binary );
    ofs << "P6\n"
        << W << " " << H << "\n255\n";
    for ( int i = 0; i < W * H; ++i ) {
        double iter = scaleCol*nbIters[i];
        unsigned char r = (unsigned char)(256 - (unsigned (iter*256.) & 0xFF));
        unsigned char b = (unsigned char)(256 - (unsigned (iter*65536) & 0xFF));
        unsigned char g = (unsigned char)(256 - (unsigned( iter*16777216) & 0xFF));
        ofs << r << g << b;
    }
    ofs.close();
}

int main(int argc, char *argv[] ) 
 { 
    const int W = 800; //800
    const int H = 600;
    // Normalement, pour un bon rendu, il faudrait le nombre d'itérations
    // ci--dessous :
    //const int maxIter = 16777216;
    const int maxIter = 8*65536;

    MPI_Init(&argc,&argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int nbproc;
    MPI_Comm_size(MPI_COMM_WORLD, &nbproc);

    auto iters = computeMandelbrotSet( W, H, maxIter );
    //std::cout << "007 : ["<<rank<<"] sorti de la fonction de calcul"<<std::endl;
    if (rank == 0){
        //std::cout<< "006 : Calcul de l'image" << std::endl;
        savePicture("mandelbrot.tga", W, H, iters, maxIter);
    }


    MPI_Finalize();

    //std::cout << "008 : ["<<rank<<"] sorti de la fonction de MPI"<<std::endl;
    return 0;
 }
 
 /*   
Question 1 (GATHER) : 
1cpu : 17.5
2cpu : 8.91
3cpu : 7.04
4cpu : 4.95
5cpu : 3.7
6cpu : 4.4
8cpu : 9.7
*/