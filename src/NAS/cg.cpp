/*
MIT License

Copyright (c) 2021 Parallel Applications Modelling Group - GMAP 
	GMAP website: https://gmap.pucrs.br
	
	Pontifical Catholic University of Rio Grande do Sul (PUCRS)
	Av. Ipiranga, 6681, Porto Alegre - Brazil, 90619-900

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

------------------------------------------------------------------------------

The original NPB 3.4.1 version was written in Fortran and belongs to: 
	http://www.nas.nasa.gov/Software/NPB/

Authors of the Fortran code:
	M. Yarrow
	C. Kuszmaul
	H. Jin

------------------------------------------------------------------------------

The serial C++ version is a translation of the original NPB 3.4.1
Serial C++ version: https://github.com/GMAP/NPB-CPP/tree/master/NPB-SER

Authors of the C++ code: 
	Dalvan Griebler <dalvangriebler@gmail.com>
	Gabriell Araujo <hexenoften@gmail.com>
 	Júnior Löff <loffjh@gmail.com>

------------------------------------------------------------------------------

The OpenMP version is a parallel implementation of the serial C++ version
OpenMP version: https://github.com/GMAP/NPB-CPP/tree/master/NPB-OMP

Authors of the OpenMP code:
	Júnior Löff <loffjh@gmail.com>
	
*/

#include "cg.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <sstream>

namespace nas_cg {
namespace {
using boolean = int;
constexpr int TRUE = 1, FALSE = 0;
constexpr double r23 = 0x1p-23, r46 = 0x1p-46;
constexpr double t23 = 0x1p23, t46 = 0x1p46;

struct Generator {
    int nonzer;
    double shift, rcond;
    double tran = 314159265.0, amult = 1220703125.0;
    explicit Generator(const Config& c) : nonzer(c.nonzer), shift(c.shift), rcond(c.rcond) {}
    static int icnvrt(double x, int ipwr2) { return static_cast<int>(ipwr2 * x); }
    void makea(int n, int nz, float a[], int colidx[], int rowstr[], int firstrow, int lastrow,
               int firstcol, int lastcol, int arow[], int acol[], float aelt[], int iv[]);
    void sparse(float a[], int colidx[], int rowstr[], int n, int nz, int nozer, int arow[],
                int acol[], float aelt[], int firstrow, int lastrow, int nzloc[], double rcond, double shift);
    void sprnvc(int n, int nz, int nn1, double v[], int iv[]);
    static void vecset(int n, double v[], int iv[], int* nzv, int i, double val);
};

double randlc(double *x, double a) {
    double t1, t2, t3, t4, a1, a2, x1, x2, z;

    /*
	 * ---------------------------------------------------------------------
	 * break A into two parts such that A = 2^23 * A1 + A2.
	 * ---------------------------------------------------------------------
	 */
    t1 = r23 * a;
    a1 = (int)t1;
    a2 = a - t23 * a1;

    /*
	 * ---------------------------------------------------------------------
	 * break X into two parts such that X = 2^23 * X1 + X2, compute
	 * Z = A1 * X2 + A2 * X1  (mod 2^23), and then
	 * X = 2^23 * Z + A2 * X2  (mod 2^46).
	 * ---------------------------------------------------------------------
	 */
    t1 = r23 * (*x);
    x1 = (int)t1;
    x2 = (*x) - t23 * x1;
    t1 = a1 * x2 + a2 * x1;
    t2 = (int)(r23 * t1);
    z = t1 - t23 * t2;
    t3 = t23 * z + a2 * x2;
    t4 = (int)(r46 * t3);
    (*x) = t3 - t46 * t4;

    return (r46 * (*x));
}

void Generator::makea(int n, int nz, float a[], int colidx[], int rowstr[], int firstrow, int lastrow, int /*firstcol*/, int /*lastcol*/, int arow[], int acol[], float aelt[], int iv[]) {
    int iouter, ivelt, nzv, nn1;
    std::vector<int> ivc(nonzer + 1);
    std::vector<double> vc(nonzer + 1);

    /*
	 * --------------------------------------------------------------------
	 * nonzer is approximately  (int(sqrt(nnza /n)));
	 * --------------------------------------------------------------------
	 * nn1 is the smallest power of two not less than n
	 * --------------------------------------------------------------------
	 */
    nn1 = 1;
    do {
        nn1 = 2 * nn1;
    } while (nn1 < n);

    /*
	 * -------------------------------------------------------------------
	 * generate nonzero positions and save for the use in sparse
	 * -------------------------------------------------------------------
	 */
    for (iouter = 0; iouter < n; iouter++) {
        nzv = nonzer;
        sprnvc(n, nzv, nn1, vc.data(), ivc.data());
        vecset(n, vc.data(), ivc.data(), &nzv, iouter + 1, 0.5);
        arow[iouter] = nzv;
        for (ivelt = 0; ivelt < nzv; ivelt++) {
            acol[(iouter) * (nonzer + 1) + (ivelt)] = ivc[ivelt] - 1;
            aelt[(iouter) * (nonzer + 1) + (ivelt)] = vc[ivelt];
        }
    }

    /*
	 * ---------------------------------------------------------------------
	 * ... make the sparse matrix from list of elements with duplicates
	 * (iv is used as  workspace)
	 * ---------------------------------------------------------------------
	 */
    sparse(a, colidx, rowstr, n, nz, nonzer, arow, acol, aelt, firstrow, lastrow, iv, rcond, shift);
}

/*
 * ---------------------------------------------------------------------
 * rows range from firstrow to lastrow
 * the rowstr pointers are defined for nrows = lastrow-firstrow+1 values
 * ---------------------------------------------------------------------
 */
void Generator::sparse(float a[], int colidx[], int rowstr[], int n, int nz, int /*nozer*/, int arow[], int acol[], float aelt[], int firstrow, int lastrow, int nzloc[], double rcond, double shift) {
    int nrows;

    /*
	 * ---------------------------------------------------
	 * generate a sparse matrix from a list of
	 * [col, row, element] tri
	 * ---------------------------------------------------
	 */
    int i, j, j1, j2, nza, k, kk, nzrow, jcol;
    double size, scale, ratio, va;
    boolean goto_40;

    /*
	 * --------------------------------------------------------------------
	 * how many rows of result
	 * --------------------------------------------------------------------
	 */
    nrows = lastrow - firstrow + 1;

    /*
	 * --------------------------------------------------------------------
	 * ...count the number of triples in each row
	 * --------------------------------------------------------------------
	 */
    for (j = 0; j < nrows + 1; j++) {
        rowstr[j] = 0;
    }
    for (i = 0; i < n; i++) {
        for (nza = 0; nza < arow[i]; nza++) {
            j = acol[(i) * (nonzer + 1) + (nza)] + 1;
            rowstr[j] = rowstr[j] + arow[i];
        }
    }
    rowstr[0] = 0;
    for (j = 1; j < nrows + 1; j++) {
        rowstr[j] = rowstr[j] + rowstr[j - 1];
    }
    nza = rowstr[nrows] - 1;

    /*
	 * ---------------------------------------------------------------------
	 * ... rowstr(j) now is the location of the first nonzero
	 * of row j of a
	 * ---------------------------------------------------------------------
	 */
    if (nza > nz) {
        throw std::runtime_error("NAS CG generator exceeded matrix capacity");
    }

    /*
	 * ---------------------------------------------------------------------
	 * ... preload data pages
	 * ---------------------------------------------------------------------
	 */
    for (j = 0; j < nrows; j++) {
        for (k = rowstr[j]; k < rowstr[j + 1]; k++) {
            a[k] = 0.0;
            colidx[k] = -1;
        }
        nzloc[j] = 0;
    }

    /*
	 * ---------------------------------------------------------------------
	 * ... generate actual values by summing duplicates
	 * ---------------------------------------------------------------------
	 */
    size = 1.0;
    ratio = pow(rcond, (1.0 / (double)(n)));
    for (i = 0; i < n; i++) {
        for (nza = 0; nza < arow[i]; nza++) {
            j = acol[(i) * (nonzer + 1) + (nza)];

            scale = size * aelt[(i) * (nonzer + 1) + (nza)];
            for (nzrow = 0; nzrow < arow[i]; nzrow++) {
                jcol = acol[(i) * (nonzer + 1) + (nzrow)];
                va = aelt[(i) * (nonzer + 1) + (nzrow)] * scale;

                /*
				 * --------------------------------------------------------------------
				 * ... add the identity * rcond to the generated matrix to bound
				 * the smallest eigenvalue from below by rcond
				 * --------------------------------------------------------------------
				 */
                if (jcol == j && j == i) {
                    va = va + rcond - shift;
                }

                goto_40 = FALSE;
                for (k = rowstr[j]; k < rowstr[j + 1]; k++) {
                    if (colidx[k] > jcol) {
                        /*
						 * ----------------------------------------------------------------
						 * ... insert colidx here orderly
						 * ----------------------------------------------------------------
						 */
                        for (kk = rowstr[j + 1] - 2; kk >= k; kk--) {
                            if (colidx[kk] > -1) {
                                a[kk + 1] = a[kk];
                                colidx[kk + 1] = colidx[kk];
                            }
                        }
                        colidx[k] = jcol;
                        a[k] = 0.0;
                        goto_40 = TRUE;
                        break;
                    } else if (colidx[k] == -1) {
                        colidx[k] = jcol;
                        goto_40 = TRUE;
                        break;
                    } else if (colidx[k] == jcol) {
                        /*
						 * --------------------------------------------------------------
						 * ... mark the duplicated entry
						 * -------------------------------------------------------------
						 */
                        nzloc[j] = nzloc[j] + 1;
                        goto_40 = TRUE;
                        break;
                    }
                }
                if (goto_40 == FALSE) {
                    throw std::runtime_error("NAS CG internal sparse insertion error");
                }
                a[k] = a[k] + va;
            }
        }
        size = size * ratio;
    }

    /*
	 * ---------------------------------------------------------------------
	 * ... remove empty entries and generate final results
	 * ---------------------------------------------------------------------
	 */
    for (j = 1; j < nrows; j++) {
        nzloc[j] = nzloc[j] + nzloc[j - 1];
    }

    for (j = 0; j < nrows; j++) {
        if (j > 0) {
            j1 = rowstr[j] - nzloc[j - 1];
        } else {
            j1 = 0;
        }
        j2 = rowstr[j + 1] - nzloc[j];
        nza = rowstr[j];
        for (k = j1; k < j2; k++) {
            a[k] = a[nza];
            colidx[k] = colidx[nza];
            nza = nza + 1;
        }
    }
    for (j = 1; j < nrows + 1; j++) {
        rowstr[j] = rowstr[j] - nzloc[j - 1];
    }
    nza = rowstr[nrows] - 1;
}

/*
 * ---------------------------------------------------------------------
 * generate a sparse n-vector (v, iv)
 * having nzv nonzeros
 *
 * mark(i) is set to 1 if position i is nonzero.
 * mark is all zero on entry and is reset to all zero before exit
 * this corrects a performance bug found by John G. Lewis, caused by
 * reinitialization of mark on every one of the n calls to sprnvc
 * ---------------------------------------------------------------------
 */
void Generator::sprnvc(int n, int nz, int nn1, double v[], int iv[]) {
    int nzv, ii, i;
    double vecelt, vecloc;

    nzv = 0;

    while (nzv < nz) {
        vecelt = randlc(&tran, amult);

        /*
		 * --------------------------------------------------------------------
		 * generate an integer between 1 and n in a portable manner
		 * --------------------------------------------------------------------
		 */
        vecloc = randlc(&tran, amult);
        i = icnvrt(vecloc, nn1) + 1;
        if (i > n) {
            continue;
        }

        /*
		 * --------------------------------------------------------------------
		 * was this integer generated already?
		 * --------------------------------------------------------------------
		 */
        boolean was_gen = FALSE;
        for (ii = 0; ii < nzv; ii++) {
            if (iv[ii] == i) {
                was_gen = TRUE;
                break;
            }
        }
        if (was_gen) {
            continue;
        }
        v[nzv] = vecelt;
        iv[nzv] = i;
        nzv = nzv + 1;
    }
}

/*
 * --------------------------------------------------------------------
 * set ith element of sparse vector (v, iv) with
 * nzv nonzeros to val
 * --------------------------------------------------------------------
 */
void Generator::vecset(int /*n*/, double v[], int iv[], int *nzv, int i, double val) {
    int k;
    boolean set;

    set = FALSE;
    for (k = 0; k < *nzv; k++) {
        if (iv[k] == i) {
            v[k] = val;
            set = TRUE;
        }
    }
    if (set == FALSE) {
        v[*nzv] = val;
        iv[*nzv] = i;
        *nzv = *nzv + 1;
    }
}

} // namespace

void validate_data(const Data& data) {
    const auto& c = data.config;
    if (c.rows == 0 || c.rows > (uint64_t(std::numeric_limits<int>::max()) - 4095) / 4 - 16 ||
        c.cg_iterations == 0 || c.iterations == 0 || !std::isfinite(c.shift) ||
        !std::isfinite(c.rcond) || c.rcond <= 0 || c.rcond > 1 ||
        data.rowstr.size() != std::size_t(c.rows) + 1 || data.rowstr.front() != 0 ||
        data.a.size() != data.colidx.size() ||
        data.colidx.size() > (uint64_t(std::numeric_limits<int>::max()) - 4095) / 4 - 16 ||
        data.rowstr.back() < 0 || std::size_t(data.rowstr.back()) != data.a.size())
        throw std::invalid_argument("Invalid NAS CG dimensions/CSR/configuration");
    for (std::size_t row = 0; row < c.rows; ++row)
        if (data.rowstr[row] < 0 || data.rowstr[row] > data.rowstr[row + 1])
            throw std::invalid_argument("Invalid NAS CG row offsets");
    for (std::size_t edge = 0; edge < data.a.size(); ++edge)
        if (data.colidx[edge] < 0 || uint32_t(data.colidx[edge]) >= c.rows || !std::isfinite(data.a[edge]))
            throw std::invalid_argument("Invalid NAS CG column/value");
}

Data make_data(const Config& config) {
    const uint64_t nz = uint64_t(config.rows) * (uint64_t(config.nonzer) + 1) *
                        (uint64_t(config.nonzer) + 1);
    const uint64_t limit = (uint64_t(std::numeric_limits<int>::max()) - 4095) / 4 - 16;
    if (config.rows == 0 || config.rows > limit || config.nonzer == 0 ||
        config.nonzer > 64 || config.nonzer > config.rows || nz > limit ||
        config.cg_iterations == 0 || config.iterations == 0 || !std::isfinite(config.shift) ||
        !std::isfinite(config.rcond) || config.rcond <= 0 || config.rcond > 1)
        throw std::invalid_argument("Invalid NAS CG generator configuration");
    Data data{config, std::vector<int>(std::size_t(config.rows) + 1),
              std::vector<int>(nz), std::vector<float>(nz)};
    std::vector<int> iv(config.rows), arow(config.rows);
    const auto naz = std::size_t(config.rows) * (config.nonzer + 1);
    std::vector<int> acol(naz);
    std::vector<float> aelt(naz);
    Generator generator(config);
    randlc(&generator.tran, generator.amult); // Original initial discarded draw.
    generator.makea(config.rows, nz, data.a.data(), data.colidx.data(), data.rowstr.data(),
                    0, config.rows - 1, 0, config.rows - 1,
                    arow.data(), acol.data(), aelt.data(), iv.data());
    const auto actual_nz = std::size_t(data.rowstr.back());
    data.colidx.resize(actual_nz);
    data.a.resize(actual_nz);
    validate_data(data);
    return data;
}

Output make_output(const Data& data) {
    validate_data(data);
    const auto n = std::size_t(data.config.rows) + 2;
    Output output{std::vector<float>(n), std::vector<float>(n), std::vector<float>(n),
                  std::vector<float>(n), std::vector<float>(n),
                  std::vector<double>(data.config.iterations), std::vector<double>(data.config.iterations)};
    std::fill_n(output.x.begin(), data.config.rows + 1, 1.0f);
    return output;
}

namespace {

template<bool UseDTU>
void spmv(const Data& data, const float* vector, VectorGather* gather, float* result) {
    const float* view = nullptr;
    if constexpr (UseDTU)
        view = gather->publish(vector, data.config.rows);
    for (std::size_t row = 0; row < data.config.rows; ++row) {
        float suml = 0.0f;
        for (int k = data.rowstr[row]; k < data.rowstr[row + 1]; ++k) {
            if constexpr (UseDTU)
                suml += data.a[k] * view[k];
            else
                suml += data.a[k] * vector[data.colidx[k]];
        }
        result[row] = suml;
    }
}

template<bool UseDTU>
void run(const Data& data, VectorGather* gather, Output& out) {
    const auto n = data.config.rows;
    for (uint32_t iteration = 0; iteration < data.config.iterations; ++iteration) {
        float rho = 0.0f, sum = 0.0f;
        for (uint32_t j = 0; j < n + 1; ++j) {
            out.q[j] = 0.0f;
            out.z[j] = 0.0f;
            out.r[j] = out.x[j];
            out.p[j] = out.r[j];
        }
        for (uint32_t j = 0; j < n; ++j)
            rho += out.r[j] * out.r[j];
        for (uint32_t cgit = 0; cgit < data.config.cg_iterations; ++cgit) {
            float d = 0.0f;
            const float rho0 = rho;
            rho = 0.0f;
            spmv<UseDTU>(data, out.p.data(), gather, out.q.data());
            for (uint32_t j = 0; j < n; ++j)
                d += out.p[j] * out.q[j];
            const float alpha = rho0 / d;
            for (uint32_t j = 0; j < n; ++j) {
                out.z[j] += alpha * out.p[j];
                out.r[j] -= alpha * out.q[j];
                rho += out.r[j] * out.r[j];
            }
            const float beta = rho / rho0;
            for (uint32_t j = 0; j < n; ++j)
                out.p[j] = out.r[j] + beta * out.p[j];
        }
        spmv<UseDTU>(data, out.z.data(), gather, out.r.data());
        for (uint32_t j = 0; j < n; ++j) {
            const float suml = out.x[j] - out.r[j];
            sum += suml * suml;
        }
        out.rnorm_history[iteration] = std::sqrt(sum);
        double norm_temp1 = 0.0, norm_temp2 = 0.0;
        for (uint32_t j = 0; j < n; ++j) {
            norm_temp1 += out.x[j] * out.z[j];
            norm_temp2 += out.z[j] * out.z[j];
        }
        norm_temp2 = 1.0 / std::sqrt(norm_temp2);
        out.zeta_history[iteration] = data.config.shift + 1.0 / norm_temp1;
        for (uint32_t j = 0; j < n; ++j)
            out.x[j] = norm_temp2 * out.z[j];
    }
}

} // namespace

void run_cpu(const Data& data, Output& output) { run<false>(data, nullptr, output); }
void run_dtu(const Data& data, VectorGather& gather, Output& output) {
    run<true>(data, &gather, output);
}

DTLProgram make_dtl_program(const Data& data) {
    validate_data(data);
    const uint32_t stream = std::max<uint64_t>(16, ((data.colidx.size() + 15) / 16) * 16);
    const auto bytes = [](std::size_t n) { return ((n * 4 + 4095) / 4096) * 4096; };
    std::ostringstream program;
    program << "DTL<" << bytes(std::size_t(data.config.rows) + 2) << "> cg_vector = {\n"
            << "  metadata_stream<4, " << bytes(stream) << "> columns;\n"
            << "  for (int edge = 0; edge < " << stream << "; edge++) { out = columns[edge]; }\n"
            << "}\nactive cg_vector;\n";
    return {program.str(), stream};
}

void verify_results(const Data& data, const Output& expected, const Output& actual) {
    const auto compare = [](const auto& a, const auto& b) {
        if (a.size() != b.size()) throw std::runtime_error("NAS CG result size mismatch");
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (!std::isfinite(a[i]) || !std::isfinite(b[i]) ||
                std::memcmp(&a[i], &b[i], sizeof(a[i])) != 0)
                throw std::runtime_error("NAS CG CPU/DTU result mismatch at element " + std::to_string(i));
        }
    };
    for (const auto& pair : {std::make_pair(&expected.x, &actual.x), std::make_pair(&expected.z, &actual.z),
                           std::make_pair(&expected.p, &actual.p), std::make_pair(&expected.q, &actual.q),
                           std::make_pair(&expected.r, &actual.r)})
        compare(*pair.first, *pair.second);
    compare(expected.rnorm_history, actual.rnorm_history);
    compare(expected.zeta_history, actual.zeta_history);
    // Independently recompute A*z, the original explicit final residual product.
    for (std::size_t row = 0; row < data.config.rows; ++row) {
        float sum = 0.0f;
        for (int k = data.rowstr[row]; k < data.rowstr[row + 1]; ++k)
            sum += data.a[k] * actual.z[data.colidx[k]];
        if (std::memcmp(&sum, &actual.r[row], sizeof(float)) != 0)
            throw std::runtime_error("NAS CG final A*z verification failed");
    }
}

uint64_t checksum(const Output& output) {
    uint64_t hash = 14695981039346656037ULL;
    const auto append = [&hash](const auto& values) {
        for (const auto& value : values) {
            uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(value));
            for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
                hash ^= (bits >> (8 * byte)) & 0xff;
                hash *= 1099511628211ULL;
            }
        }
    };
    for (const auto* values : {&output.x, &output.z, &output.p, &output.q, &output.r}) append(*values);
    append(output.rnorm_history);
    append(output.zeta_history);
    return hash;
}

} // namespace nas_cg
