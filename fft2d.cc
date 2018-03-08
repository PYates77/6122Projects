// Distributed two-dimensional Discrete FFT transform
// Paul Yates

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <future>

#include "Complex.h"
#include "InputImage.h"

const double PI = 4*atan(1);

constexpr unsigned int NUMTHREADS = 4;

using namespace std;

//undergrad students can assume NUMTHREADS will evenly divide the number of rows in tested images
//graduate students should assume NUMTHREADS will not always evenly divide the number of rows in tested images.
// I will test with a different image than the one given

void Transform1D(const Complex* h, const int w, Complex* H);
void TransformColumns(const Complex* in, const int w, const int h, Complex* H, const int o);

void Transform2D(const char* inputFN)
{ // Do the 2D transform here.
    // 1) Use the InputImage object to read in the Tower.txt file and
    //    find the width/height of the input image.
    // 2) Create a vector of complex objects of size width * height to hold
    //    values calculated
    // 3) Do the individual 1D transforms on the rows assigned to each thread
    // 4) Force each thread to wait until all threads have completed their row calculations
    //    prior to starting column calculations
    // 5) Perform column calculations
    // 6) Wait for all column calculations to complete
    // 7) Use SaveImageData() to output the final results

    InputImage image(inputFN);  // Create the helper object for reading the image
    // Step (1) in the comments is the line above.
    // Your code here, steps 2-7
    Complex* data = image.GetImageData();
    int h = image.GetHeight();
    int w = image.GetWidth();
    Complex* intermediate = new Complex[w*h];
    Complex* answer = new Complex[w*h];

    std::future<Complex> intermediate[h*w];

    //row calculations
    for(int i=0;i<h;++i){
        Transform1D(data+w*i, w, intermediate+i*w);
    }

    image.SaveImageData("after1d_test.txt",intermediate, w,h);

    //column calculations
    for(int i=0;i<w;++i){
        TransformColumns(intermediate, w, h, answer, i);
    }

    image.SaveImageData("after2d_test.txt",answer,w,h);
    delete intermediate;
    delete answer;
}

void Transform1D(const Complex* h, const int w, Complex* H)
{
    // Implement a simple 1-d DFT using the double summation equation
    // given in the assignment handout.  h is the time-domain input
    // data, w is the width (N), and H is the output array.
    // TODO: fancy threading support?
    for(int n=0; n<w; ++n) {
        for (int k = 0; k < w; ++k) {
            H[n]=H[n]+h[k]*Complex(cos(2 * PI * k * n / w), -sin(2 * PI * k * n / w));
        }
    }
}

void TransformColumns(const Complex* in, const int w, const int h, Complex* H, const int o){
    // Does the same thing as Transform1D except uses an offset o to decide which column to transform
    // Should be given the ENTIRE array, not just a single row
    for(int n=0; n<h; ++n){
        for (int k=0; k<h; ++k){
            H[w*n+o] = H[w*n+o] + in[w*k+o]*Complex(cos(2 * PI * k * n / w), -sin(2 * PI * k * n / w));
        }
    }
}

int main(int argc, char** argv)
{
    string fn("../Tower.txt"); // default file name
    if (argc > 1) fn = string(argv[1]);  // if name specified on cmd line
    Transform2D(fn.c_str()); // Perform the transform.
}  
  

  
