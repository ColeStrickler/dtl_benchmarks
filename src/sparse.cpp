#include "sparse.hpp"

void spmv_csr(int nrows,
              const int *row_ptr,
              const int *col_idx,
              const float *values,
              const float *x,
              float *y)
{
    for (int row = 0; row < nrows; row++) {
        float sum = 0.0f;
        for (int k = row_ptr[row];
             k < row_ptr[row + 1];
             k++) {
            sum += values[k] * x[col_idx[k]]; // x[1], x[1000], x[25000]
        }
        y[row] = sum;
    }
}

void spmv_dtu(int nrows,
              const int *row_ptr,
              const float *values,
              const float *x_dtu,
              float *y)
{
    for (int row = 0; row < nrows; row++) {
        float sum = 0.0f;
        for (int k = row_ptr[row];
             k < row_ptr[row + 1];
             k++) {
            sum += values[k] * x_dtu[k]; // x[1], x[2], x[3] 
        }
        y[row] = sum;
    }
}


static uint32_t xorshift32(uint32_t* state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

int generate_csr_random_rows(
    int nrows,
    int ncols,
    int min_nnz_per_row,
    int max_nnz_per_row,
    int* row_ptr,
    int* col_idx,
    float* values,
    uint32_t seed)
{
    if (min_nnz_per_row < 0) return -1;
    if (max_nnz_per_row > ncols) return -1;
    if (min_nnz_per_row > max_nnz_per_row) return -1;
    if (seed == 0) seed = 1;

    row_ptr[0] = 0;

    for (int row = 0; row < nrows; row++) {
        int range = max_nnz_per_row - min_nnz_per_row + 1;

        int row_nnz =
            min_nnz_per_row + (xorshift32(&seed) % range);

        row_ptr[row + 1] = row_ptr[row] + row_nnz;

        int row_start = row_ptr[row];

        for (int j = 0; j < row_nnz; j++) {
            int k = row_start + j;

            uint32_t r = xorshift32(&seed);

            col_idx[k] = r % ncols;

            values[k] = (float)((int)(r & 0xff) + 1);
        }
    }

    return row_ptr[nrows]; // total nnz
}

void generate_vector(int ncols, float *x, uint32_t seed)
{
    if (seed == 0)
        seed = 1;

    for (int i = 0; i < ncols; i++) {
        uint32_t r = xorshift32(&seed);
        x[i] = (float)((r & 0xff) + 1);
    }
}





void sddmm(
    int M,
    int D,
    const float* Q,
    const float* K,
    const int* row_ptr,
    const int* col_idx,
    float* scores)
{
    const float scale = 1.0f / std::sqrt(static_cast<float>(D));

    for (int i = 0; i < M; ++i) {
        const float* q = Q + i * D;

        for (int p = row_ptr[i]; p < row_ptr[i + 1]; ++p) {
            const int j = col_idx[p];
            const float* k = K + j * D;

            float dot = 0.0f;
            for (int d = 0; d < D; ++d)
                dot += q[d] * k[d];

            scores[p] = dot * scale;
        }
    }
}


void sddmm_dtu(
    int M,
    int D,
    const float* Q,
    const float* K_dtu,
    const int* row_ptr,
    float* scores)
{
    const float scale = 1.0f / std::sqrt(static_cast<float>(D));

    for (int i = 0; i < M; ++i) {
        const float* q = Q + i * D;

        for (int p = row_ptr[i]; p < row_ptr[i + 1]; ++p) {
            //const float* k = K + col_idx[p] * D;

            float dot = 0.0f;
            for (int d = 0; d < D; ++d)
                dot += q[d] * K_dtu[p*D+d];

            scores[p] = dot * scale;
        }
    }
}






// Performs softmax independently over each sparse CSR row.
void sparse_softmax(
    int M,
    const int* row_ptr,
    float* scores)
{
    for (int i = 0; i < M; ++i) {
        const int begin = row_ptr[i];
        const int end   = row_ptr[i + 1];

        if (begin == end)
            continue;

        float row_max = scores[begin];
        for (int p = begin + 1; p < end; ++p)
            row_max = std::max(row_max, scores[p]);

        float sum = 0.0f;
        for (int p = begin; p < end; ++p) {
            scores[p] = std::exp(scores[p] - row_max);
            sum += scores[p];
        }

        const float inverse_sum = 1.0f / sum;
        for (int p = begin; p < end; ++p)
            scores[p] *= inverse_sum;
    }
}

// P: sparse [M, N] in CSR
// V: dense  [N, DV]
// O: dense  [M, DV]
//
// O[i] = sum_j P[i,j] * V[j]
void spmm_attention(
    int M,
    int DV,
    const int* row_ptr,
    const int* col_idx,
    const float* probabilities,
    const float* V,
    float* O)
{
    std::fill(O, O + M * DV, 0.0f);

    for (int i = 0; i < M; ++i) {
        float* output_row = O + i * DV;

        for (int p = row_ptr[i]; p < row_ptr[i + 1]; ++p) {
            const float weight = probabilities[p];
            const float* value_row = V + col_idx[p] * DV;

            for (int d = 0; d < DV; ++d)
                output_row[d] += weight * value_row[d];
        }
    }
}


void spmm_attention_dtu(
    int M,
    int DV,
    const int* row_ptr,
    const float* dtu_V,
    const float* probabilities,
    float* O)
{
    std::fill(O, O + M * DV, 0.0f);

    for (int i = 0; i < M; ++i) {
        float* output_row = O + i * DV;

        for (int p = row_ptr[i]; p < row_ptr[i + 1]; ++p) {
            const float weight = probabilities[p];
            //const float* value_row = V + col_idx[p] * DV;

            for (int d = 0; d < DV; ++d)
                output_row[d] += weight * dtu_V[p*DV + d];
        }
    }
}



void sparse_attention(
    int M,
    int D,
    int DV,
    const float* Q,
    const float* K,
    const float* V,
    const int* row_ptr,
    const int* col_idx,
    float* scores,
    float* O)
{
    const int nnz = row_ptr[M];

    sddmm(M, D, Q, K, row_ptr, col_idx, scores);
    sparse_softmax(M, row_ptr, scores);
    spmm_attention(M, DV, row_ptr, col_idx, scores, V, O);
}
#include "perf.hpp"


void sparse_attention_dtu(
    int M,
    int D,
    int DV,
    const float* Q,
    const float* K_dtu,
    const int* row_ptr,
    const float* dtu_V,
    float* scores,
    float* O)
{
    const int nnz = row_ptr[M];
    PerfManager perf;

    perf.CollectCounters();
    sddmm_dtu(M, D, Q, K_dtu, row_ptr, scores);
    perf.CollectDelta();
    std::cout << "STEP1," << perf.PrintCounters() << std::endl;
    perf.ClearCounters();
    perf.CollectCounters();
    sparse_softmax(M, row_ptr, scores);
    perf.CollectDelta();
    std::cout << "STEP1," << perf.PrintCounters() << std::endl;
    perf.ClearCounters();
    perf.CollectCounters();
    spmm_attention_dtu(M, DV, row_ptr, dtu_V, scores, O);
    perf.CollectDelta();
    std::cout << "STEP1," << perf.PrintCounters() << std::endl;
    perf.ClearCounters();
}





SparseAttentionBuffers generate_sparse_attention_buffers(
    int M,
    int N,
    int D,
    int DV,
    int nonzeros_per_row,
    bool causal,
    uint32_t seed)
{
    SparseAttentionBuffers b{};

    b.M  = M;
    b.N  = N;
    b.D  = D;
    b.DV = DV;

    /*
     * First determine the exact number of nonzeros.
     */
    b.nnz = 0;

    for (int i = 0; i < M; ++i) {
        const int available_columns =
            causal ? std::min(i + 1, N) : N;

        b.nnz += std::min(nonzeros_per_row, available_columns);
    }

    /*
     * Allocate all buffers.
     */
    b.Q = static_cast<float*>(
        std::malloc(sizeof(float) * M * D));

    b.K = static_cast<float*>(
        std::malloc(sizeof(float) * N * D));

    b.V = static_cast<float*>(
        std::malloc(sizeof(float) * N * DV));

    b.row_ptr = static_cast<int*>(
        std::malloc(sizeof(int) * (M + 1)));

    b.col_idx = static_cast<int*>(
        std::malloc(sizeof(int) * b.nnz));

    b.scores = static_cast<float*>(
        std::malloc(sizeof(float) * b.nnz));

    b.O = static_cast<float*>(
        std::malloc(sizeof(float) * M * DV));

    /*
     * Check allocation.
     */
    if (!b.Q || !b.K || !b.V ||
        !b.row_ptr || !b.col_idx ||
        !b.scores || !b.O) {

        std::free(b.Q);
        std::free(b.K);
        std::free(b.V);
        std::free(b.row_ptr);
        std::free(b.col_idx);
        std::free(b.scores);
        std::free(b.O);

        return {};
    }

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> value_dist(-1.0f, 1.0f);

    /*
     * Initialize dense input buffers.
     */
    for (int i = 0; i < M * D; ++i)
        b.Q[i] = value_dist(rng);

    for (int i = 0; i < N * D; ++i)
        b.K[i] = value_dist(rng);

    for (int i = 0; i < N * DV; ++i)
        b.V[i] = value_dist(rng);

    /*
     * Temporary buffer used to select unique columns.
     */
    int* candidates = static_cast<int*>(
        std::malloc(sizeof(int) * N));

    if (!candidates) {
        std::free(b.Q);
        std::free(b.K);
        std::free(b.V);
        std::free(b.row_ptr);
        std::free(b.col_idx);
        std::free(b.scores);
        std::free(b.O);

        return {};
    }

    /*
     * Generate the CSR sparsity structure.
     */
    int p = 0;
    b.row_ptr[0] = 0;

    for (int i = 0; i < M; ++i) {
        const int available_columns =
            causal ? std::min(i + 1, N) : N;

        const int row_nnz =
            std::min(nonzeros_per_row, available_columns);

        for (int j = 0; j < available_columns; ++j)
            candidates[j] = j;

        std::shuffle(
            candidates,
            candidates + available_columns,
            rng);

        std::sort(
            candidates,
            candidates + row_nnz);

        for (int k = 0; k < row_nnz; ++k)
            b.col_idx[p++] = candidates[k];

        b.row_ptr[i + 1] = p;
    }

    std::free(candidates);

    /*
     * Initialize intermediate and output buffers.
     */
    std::memset(
        b.scores,
        0,
        sizeof(float) * b.nnz);

    std::memset(
        b.O,
        0,
        sizeof(float) * M * DV);

    return b;
}

void free_sparse_attention_buffers(
    SparseAttentionBuffers* b)
{
    if (!b)
        return;

    std::free(b->Q);
    std::free(b->K);
    std::free(b->V);
    std::free(b->row_ptr);
    std::free(b->col_idx);
    std::free(b->scores);
    std::free(b->O);

    //*b = {};
}





struct GNNBuffers {
    int V;
    int D;
    int degree;
    int E;

    int* rowptr;
    int* colidx;

    float* H;       // base feature matrix: H[V][D]
    float* H_dtu;   // DTU logical view: H_dtu[E][D] := H[colidx[e]][D]
    float* out;
};

static inline void* aligned_alloc_or_die(size_t alignment, size_t size) {
    void* p = nullptr;
    if (posix_memalign(&p, alignment, size) != 0 || p == nullptr)
        std::abort();
    return p;
}

void init_gnn_buffers(GNNBuffers* b, int V, int D, int degree) {
    b->V = V;
    b->D = D;
    b->degree = degree;
    b->E = V * degree;

    b->rowptr = (int*)aligned_alloc_or_die(64, (V + 1) * sizeof(int));
    b->colidx = (int*)aligned_alloc_or_die(64, b->E * sizeof(int));

    b->H     = (float*)aligned_alloc_or_die(64, (size_t)V * D * sizeof(float));
    b->H_dtu = (float*)aligned_alloc_or_die(64, (size_t)b->E * D * sizeof(float));
    b->out   = (float*)aligned_alloc_or_die(64, (size_t)V * D * sizeof(float));

    for (int v = 0; v <= V; v++)
        b->rowptr[v] = v * degree;

    // Deterministic pseudo-random-ish graph.
    for (int v = 0; v < V; v++) {
        for (int t = 0; t < degree; t++) {
            int e = b->rowptr[v] + t;
            b->colidx[e] = (v * 131 + t * 17 + 7) % V;
        }
    }

    for (int v = 0; v < V; v++) {
        for (int d = 0; d < D; d++) {
            b->H[v * D + d] =
                0.001f * (float)((v * 97 + d * 13 + 5) % 1009);
        }
    }

    // Software materialized DTU view for correctness/testing.
    // In real DTU run, replace this with DTU registration.
    for (int e = 0; e < b->E; e++) {
        int u = b->colidx[e];
        for (int d = 0; d < D; d++) {
            b->H_dtu[e * D + d] = b->H[u * D + d];
        }
    }

    std::memset(b->out, 0, (size_t)V * D * sizeof(float));
}

void gnn_agg(
    int V,
    int D,
    const int* rowptr,
    const float* edge_weight,
    float* H,
    const int* col_idx,
    float* out)
{
    for (int v = 0; v < V; v++) {
        float* o = out + v * D;

        for (int e = rowptr[v]; e < rowptr[v + 1]; e++) {
            int idx = col_idx[e];

            for (int d = 0; d < D; d++) {
                o[d] += H[idx*D + d];
            }
        }
    }
}



void gnn_agg_dtu(
    int V,
    int D,
    const float* edge_weight,
    const int* rowptr,
    const float* H_dtu,
    float* out)
{
    for (int v = 0; v < V; v++) {
        float* o = out + v * D;

        for (int e = rowptr[v]; e < rowptr[v + 1]; e++) {
            const float* h = H_dtu + e * D;

            for (int d = 0; d < D; d++) {
                o[d] += h[d];
            }
        }
    }
}