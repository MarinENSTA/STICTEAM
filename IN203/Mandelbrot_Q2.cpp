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
    int tag=100;
    MPI_Status status;
  
    std::vector<int> pixels(W*H);
    
    if( rank == 0 )//rank==0=>master
    {
        int count_task = 0;
     
        for(int i = 1; i < nbproc; ++i ){
            MPI_Send(&count_task, 1, MPI_INT, i , tag, globComm);
            count_task +=1;
        } 

        int tasks_received = 0;
        while(tasks_received < H) {
            std::vector<int> pixel_buffer(W+1);
            //statuscontiendralenuméroduprocayantenvoyé
            //lerésultat. . .: status.MPI_SOURCEenMPI
            MPI_Recv(&pixel_buffer[0],  W+1, MPI_INT, MPI_ANY_SOURCE, tag, globComm, &status );
            if (count_task < H)
                MPI_Send(&count_task , 1, MPI_INT, status.MPI_SOURCE, tag, globComm);

            for (int i = 0; i<W+1; i++){
                pixels[pixel_buffer[0]*W + i] = pixel_buffer[i + 1];
                
            }
            count_task +=1;
            tasks_received+=1;
        }

        //Onenvoieunsignaldeterminaisonàtouslesprocessus
        count_task =-1;
        for(int i = 1; i<nbproc; ++i ) 
            MPI_Send(&count_task, 1, MPI_INT, i, tag, globComm);
    }


    else {
        //Casoùjesuisuntravailleur
        int num_task = 0;

        while(num_task != -1){
            MPI_Recv(&num_task, 1, MPI_INT, 0, tag, globComm, &status);

            if(num_task >= 0) {
                std::vector<int> pixels_row(W+1);
                pixels_row[0] = num_task;
                computeMandelbrotSetRow(W, H, maxIter, num_task, pixels_row.data()+1);
                //Renvoielerésultatavecsonnuméro
                MPI_Send(&pixels_row[0], W+1, MPI_INT, 0, tag, globComm);
            }
        }
    }


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
question 2 (Maitre-Esclave):
1cpu esclave : 19.7
2cpu esclave : 12.8
3cpu esclave : 7.2
4cpu esclave : 5.9
5cpu esclave : 3.7
6cpu esclave : 2.9
7cpu esclave : 2.5*/