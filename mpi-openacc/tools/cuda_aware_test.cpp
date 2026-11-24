#include <mpi.h>
#include <cstdio>
#include <cuda_runtime.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int N = 4;
    float *d_buf;
    cudaMalloc(&d_buf, N * sizeof(float));

    if (rank == 0) {
        float h_vals[N] = {1,2,3,4};
        cudaMemcpy(d_buf, h_vals, sizeof(h_vals), cudaMemcpyHostToDevice);
        MPI_Send(d_buf, N, MPI_FLOAT, 1, 0, MPI_COMM_WORLD);
    } else if (rank == 1) {
        MPI_Recv(d_buf, N, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        float h_vals[N];
        cudaMemcpy(h_vals, d_buf, sizeof(h_vals), cudaMemcpyDeviceToHost);
        printf("Received: %f %f %f %f\n", h_vals[0],h_vals[1],h_vals[2],h_vals[3]);
    }

    cudaFree(d_buf);
    MPI_Finalize();
    return 0;
}