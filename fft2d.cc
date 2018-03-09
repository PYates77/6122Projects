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
void RowThreader(const Complex* in, const int h, const int w, const int o, const int rows, std::promise<Complex>* P);
void ColumnThreader(std::future<Complex>* in, const int h, const int w, const int o, const int columns, Complex* out);
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
    std::cout << "Rows: " << h << std::endl;
    std::cout << "Columns: " << w << std::endl;

    Complex* answer = new Complex[w*h];
    Complex (&data2)[h][w] = *reinterpret_cast<Complex (*)[h][w]>(image.GetImageData());

    if(NUMTHREADS > 1){
        auto* promises = new std::promise<Complex>[h*w];
        auto* futures = new std::future<Complex>[h*w];

        for (int i=0; i<h*w; ++i){
            futures[i] = promises[i].get_future();
        }
        //assign a proportional amount of threads to rows and columns
        int row_threads = (NUMTHREADS*h)/(h+w);
        int column_threads = NUMTHREADS - row_threads;
        std::cout << "Row Threads: " << row_threads << std::endl;
        std::cout << "Column Threads: " << column_threads << std::endl;

        //find the number of rows per row thread and columns per column thread
        int rows_per_thread = (h+row_threads-1)/row_threads; // equivalent to ceil(h / row_threads) thanks to integer truncation
        int columns_per_thread = (w+column_threads-1)/column_threads;
        std::cout << "Rows per Thread: " << rows_per_thread << std::endl;
        std::cout << "Columns per Thread: " << columns_per_thread << std::endl;

        //find the number of rows/columns that the final thread will have to process
        int rows_last_thread = h - (row_threads - 1) * rows_per_thread;
        int columns_last_thread = w - (column_threads - 1) * rows_per_thread;
        std::cout << "Rows last Thread: " << rows_last_thread << std::endl;
        std::cout << "Columns last Thread " << columns_last_thread << std::endl;
        std::vector<std::future<void>> futs;
        //start row threads
        for(int i=0; i<row_threads-1; i++){
            int startingRow = rows_per_thread * i;
            std::cout << "Starting thread for rows " << startingRow << " - " << startingRow+rows_per_thread << std::endl;
            futs.push_back(std::async(RowThreader,data,h,w,startingRow,rows_per_thread,promises));
        }
        //run last row thread (with modified number of rows)
        int startingRow = rows_per_thread * (row_threads-1);
        std::cout << "Starting thread for rows " << startingRow << " - " << startingRow + rows_last_thread << std::endl;
        futs.push_back(std::async(RowThreader,data,h,w,startingRow,rows_last_thread,promises));

        //start column threads
        for(int i=0; i<column_threads-1; ++i){
            int startingColumn = columns_per_thread * i;
            std::cout << "Starting thread for columns " << startingColumn << " - " << startingColumn+columns_per_thread << std::endl;
            futs.push_back(std::async(ColumnThreader,futures,h,w,startingColumn,columns_per_thread,answer));
        }
        //run last column thread (with modified number of columns)
        int startingColumn = columns_per_thread * (column_threads-1);
        std::cout << "Starting thread for columns " << startingColumn << " - " << startingColumn + columns_last_thread << std::endl;
        futs.push_back(std::async(ColumnThreader,futures,h,w,startingColumn,columns_last_thread,answer));

        //wait for threads to finish before clearing allocated memory
        for(auto it = futs.begin(); it != futs.end(); ++it){
            it->get();
        }
        delete promises;
        delete futures;
    }
    else{
        Complex* intermediate = new Complex[w*h];
        //row calculations
        for(int i=0;i<h;++i){
            Transform1D(data+w*i, w, intermediate+i*w);
        }

        image.SaveImageData("after1d_test.txt",intermediate, w,h);

        //column calculations
        for(int i=0;i<w;++i){
            TransformColumns(intermediate, w, h, answer, i);
        }

        delete intermediate;
    }
    image.SaveImageData("after2d_test.txt",answer,w,h);
    delete answer;
}

void Transform1D(const Complex* h, const int w, Complex* H)
{
    // Implement a simple 1-d DFT using the double summation equation
    // given in the assignment handout.  h is the time-domain input
    // data, w is the width (N), and H is the output array.
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

void RowThreader(const Complex* in, const int h, const int w, const int o, const int rows, std::promise<Complex>* P){
    for(int i=0; i<rows; ++i){
        int rowIndex = w*(o+i);
        for(int n=0; n<w; ++n) {
            Complex temp(0);
            for (int k=0; k < w; ++k){
               temp = temp + in[rowIndex+k]* Complex(cos(2 * PI * k * n / w), -sin(2 * PI * k * n / w));
            }
            P[rowIndex+n].set_value(temp);
        }
    }
}
void ColumnThreader(std::future<Complex>* in, const int h, const int w, const int o, const int columns, Complex* out){
    for(int i=0; i<columns; ++i){
        //preload column from futures
        Complex column[h];
        for(int f=0; f<h; ++f){
            column[f] = in[w*f+(o+i)].get();
        }
        for(int n=0; n<h; ++n){
            Complex temp(0);
            for (int k=0; k<h; ++k){
                 temp = temp + column[k]*Complex(cos(2 * PI * k * n / w), -sin(2 * PI * k * n / w));
            }
            out[w*n+(o+i)] = temp;
        }
    }
}

int main(int argc, char** argv)
{
    string fn("../Tower.txt"); // default file name
    if (argc > 1) fn = string(argv[1]);  // if name specified on cmd line
    Transform2D(fn.c_str()); // Perform the transform.
}



