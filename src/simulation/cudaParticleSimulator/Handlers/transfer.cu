#include "../cudaParticleSimulator.h"

__global__ void g2pKernel(
    float3 *pos, float3 *vel, float3 *velOld,
    float *u, float *v, float *w,
    float *uOld, float *vOld, float *wOld,
    int nx, int ny, int nz,
    float invDx, float flipRatio,
    int numParticles)
{
    int pid = blockIdx.x * blockDim.x + threadIdx.x;
    if (pid >= numParticles)
        return;

    float3 p = pos[pid];
    float3 vp = vel[pid];

    float gx = p.x * invDx;
    float gy = p.y * invDx;
    float gz = p.z * invDx;

    float pic_u = 0.0f, flip_du = 0.0f;
    {
        float ux = gx, uy = gy - 0.5f, uz = gz - 0.5f;
        int i = (int)ux, j = (int)uy, k = (int)uz;
        float fx = ux - i, fy = uy - j, fz = uz - k;

        float w000 = (1 - fx) * (1 - fy) * (1 - fz), w100 = fx * (1 - fy) * (1 - fz);
        float w010 = (1 - fx) * fy * (1 - fz), w110 = fx * fy * (1 - fz);
        float w001 = (1 - fx) * (1 - fy) * fz, w101 = fx * (1 - fy) * fz;
        float w011 = (1 - fx) * fy * fz, w111 = fx * fy * fz;

#define U_IDX(i, j, k) ((i) + (nx + 1) * ((j) + ny * (k)))
// macro
#define SAFE_U(arr, i, j, k) (((i) >= 0 && (i) <= nx && (j) >= 0 && (j) < ny && (k) >= 0 && (k) < nz) ? arr[U_IDX(i, j, k)] : 0.0f)

        pic_u = w000 * SAFE_U(u, i, j, k) + w100 * SAFE_U(u, i + 1, j, k) + w010 * SAFE_U(u, i, j + 1, k) + w110 * SAFE_U(u, i + 1, j + 1, k) + w001 * SAFE_U(u, i, j, k + 1) + w101 * SAFE_U(u, i + 1, j, k + 1) + w011 * SAFE_U(u, i, j + 1, k + 1) + w111 * SAFE_U(u, i + 1, j + 1, k + 1);
        // float partial_flip_000 = w000 * SAFE_U(u, i, j, k) - w000 * SAFE_U(uOld, i, j, k);
        // float partial_flip_100 = w100 * SAFE_U(u, i + 1, j, k) - w100 * SAFE_U(uOld, i + 1, j, k);
        // float partial_flip_010 = w010 * SAFE_U(u, i, j + 1, k) - w010 * SAFE_U(uOld, i, j + 1, k);
        // float partial_flip_110 = w110 * SAFE_U(u, i + 1, j + 1, k) - w110 * SAFE_U(uOld, i + 1, j + 1, k);
        // float partial_flip_001 = w001 * SAFE_U(u, i, j, k + 1) - w001 * SAFE_U(uOld, i, j, k + 1);
        // float partial_flip_101 = w101 * SAFE_U(u, i + 1, j, k + 1) - w101 * SAFE_U(uOld, i + 1, j, k + 1);
        // float partial_flip_011 = w011 * SAFE_U(u, i, j + 1, k + 1) - w011 * SAFE_U(uOld, i, j + 1, k + 1);
        // float partial_flip_111 = w111 * SAFE_U(u, i + 1, j + 1, k + 1) - w111 * SAFE_U(uOld, i + 1, j + 1, k + 1);
        flip_du = w000 * SAFE_U(u, i, j, k) - w000 * SAFE_U(uOld, i, j, k) + w100 * SAFE_U(u, i + 1, j, k) - w100 * SAFE_U(uOld, i + 1, j, k) + w010 * SAFE_U(u, i, j + 1, k) - w010 * SAFE_U(uOld, i, j + 1, k) + w110 * SAFE_U(u, i + 1, j + 1, k) - w110 * SAFE_U(uOld, i + 1, j + 1, k) + w001 * SAFE_U(u, i, j, k + 1) - w001 * SAFE_U(uOld, i, j, k + 1) + w101 * SAFE_U(u, i + 1, j, k + 1) - w101 * SAFE_U(uOld, i + 1, j, k + 1) + w011 * SAFE_U(u, i, j + 1, k + 1) - w011 * SAFE_U(uOld, i, j + 1, k + 1) + w111 * SAFE_U(u, i + 1, j + 1, k + 1) - w111 * SAFE_U(uOld, i + 1, j + 1, k + 1);

#undef U_IDX
#undef SAFE_U
    }

    float pic_v = 0.0f, flip_dv = 0.0f;
    {
        float vx = gx - 0.5f, vy = gy, vz = gz - 0.5f;
        int i = (int)vx, j = (int)vy, k = (int)vz;
        float fx = vx - i, fy = vy - j, fz = vz - k;

        float w000 = (1 - fx) * (1 - fy) * (1 - fz), w100 = fx * (1 - fy) * (1 - fz);
        float w010 = (1 - fx) * fy * (1 - fz), w110 = fx * fy * (1 - fz);
        float w001 = (1 - fx) * (1 - fy) * fz, w101 = fx * (1 - fy) * fz;
        float w011 = (1 - fx) * fy * fz, w111 = fx * fy * fz;

#define V_IDX(i, j, k) ((i) + nx * ((j) + (ny + 1) * (k)))
#define SAFE_V(arr, i, j, k) (((i) >= 0 && (i) < nx && (j) >= 0 && (j) <= ny && (k) >= 0 && (k) < nz) ? arr[V_IDX(i, j, k)] : 0.0f)

        pic_v = w000 * SAFE_V(v, i, j, k) + w100 * SAFE_V(v, i + 1, j, k) + w010 * SAFE_V(v, i, j + 1, k) + w110 * SAFE_V(v, i + 1, j + 1, k) + w001 * SAFE_V(v, i, j, k + 1) + w101 * SAFE_V(v, i + 1, j, k + 1) + w011 * SAFE_V(v, i, j + 1, k + 1) + w111 * SAFE_V(v, i + 1, j + 1, k + 1);
        // evito di creare più var (appesantisce), illeggibile, ma dovrebbe andare meglio
        //  float partial_flip_000 = w000 * SAFE_V(v, i, j, k) - w000 * SAFE_V(vOld, i, j, k);
        //  float partial_flip_100 = w100 * SAFE_V(v, i + 1, j, k) - w100 * SAFE_V(vOld, i + 1, j, k);
        //  float partial_flip_010 = w010 * SAFE_V(v, i, j + 1, k) - w010 * SAFE_V(vOld, i, j + 1, k);
        //  float partial_flip_110 = w110 * SAFE_V(v, i + 1, j + 1, k) - w110 * SAFE_V(vOld, i + 1, j + 1, k);
        //  float partial_flip_001 = w001 * SAFE_V(v, i, j, k + 1) - w001 * SAFE_V(vOld, i, j, k + 1);
        //  float partial_flip_101 = w101 * SAFE_V(v, i + 1, j, k + 1) - w101 * SAFE_V(vOld, i + 1, j, k + 1);
        //  float partial_flip_011 = w011 * SAFE_V(v, i, j + 1, k + 1) - w011 * SAFE_V(vOld, i, j + 1, k + 1);
        //  float partial_flip_111 = w111 * SAFE_V(v, i + 1, j + 1, k + 1) - w111 * SAFE_V(vOld, i + 1, j + 1, k + 1);
        flip_dv = w000 * (SAFE_V(v, i, j, k) - SAFE_V(vOld, i, j, k)) + w100 * (SAFE_V(v, i + 1, j, k) - SAFE_V(vOld, i + 1, j, k)) + w010 * (SAFE_V(v, i, j + 1, k) - SAFE_V(vOld, i, j + 1, k)) + w110 * (SAFE_V(v, i + 1, j + 1, k) - SAFE_V(vOld, i + 1, j + 1, k)) + w001 * (SAFE_V(v, i, j, k + 1) - SAFE_V(vOld, i, j, k + 1)) + w101 * (SAFE_V(v, i + 1, j, k + 1) - SAFE_V(vOld, i + 1, j, k + 1)) + w011 * (SAFE_V(v, i, j + 1, k + 1) - SAFE_V(vOld, i, j + 1, k + 1)) + w111 * (SAFE_V(v, i + 1, j + 1, k + 1) - SAFE_V(vOld, i + 1, j + 1, k + 1));

#undef V_IDX
#undef SAFE_V
    }

    float pic_w = 0.0f, flip_dw = 0.0f;
    {
        float wx = gx - 0.5f, wy = gy - 0.5f, wz = gz;
        int i = (int)wx, j = (int)wy, k = (int)wz;
        float fx = wx - i, fy = wy - j, fz = wz - k;

        float w000 = (1 - fx) * (1 - fy) * (1 - fz), w100 = fx * (1 - fy) * (1 - fz);
        float w010 = (1 - fx) * fy * (1 - fz), w110 = fx * fy * (1 - fz);
        float w001 = (1 - fx) * (1 - fy) * fz, w101 = fx * (1 - fy) * fz;
        float w011 = (1 - fx) * fy * fz, w111 = fx * fy * fz;

#define W_IDX(i, j, k) ((i) + nx * ((j) + ny * (k)))
#define SAFE_W(arr, i, j, k) (((i) >= 0 && (i) < nx && (j) >= 0 && (j) < ny && (k) >= 0 && (k) <= nz) ? arr[W_IDX(i, j, k)] : 0.0f)

        pic_w = w000 * SAFE_W(w, i, j, k) + w100 * SAFE_W(w, i + 1, j, k) + w010 * SAFE_W(w, i, j + 1, k) + w110 * SAFE_W(w, i + 1, j + 1, k) + w001 * SAFE_W(w, i, j, k + 1) + w101 * SAFE_W(w, i + 1, j, k + 1) + w011 * SAFE_W(w, i, j + 1, k + 1) + w111 * SAFE_W(w, i + 1, j + 1, k + 1);

        flip_dw = w000 * (SAFE_W(w, i, j, k) - SAFE_W(wOld, i, j, k)) + w100 * (SAFE_W(w, i + 1, j, k) - SAFE_W(wOld, i + 1, j, k)) + w010 * (SAFE_W(w, i, j + 1, k) - SAFE_W(wOld, i, j + 1, k)) + w110 * (SAFE_W(w, i + 1, j + 1, k) - SAFE_W(wOld, i + 1, j + 1, k)) + w001 * (SAFE_W(w, i, j, k + 1) - SAFE_W(wOld, i, j, k + 1)) + w101 * (SAFE_W(w, i + 1, j, k + 1) - SAFE_W(wOld, i + 1, j, k + 1)) + w011 * (SAFE_W(w, i, j + 1, k + 1) - SAFE_W(wOld, i, j + 1, k + 1)) + w111 * (SAFE_W(w, i + 1, j + 1, k + 1) - SAFE_W(wOld, i + 1, j + 1, k + 1));

#undef W_IDX
#undef SAFE_W
    }

    // BLEND FLIP PIC
    velOld[pid] = vel[pid]; // salva per old vel

    vel[pid].x = flipRatio * (vp.x + flip_du) + (1.0f - flipRatio) * pic_u;
    vel[pid].y = flipRatio * (vp.y + flip_dv) + (1.0f - flipRatio) * pic_v;
    vel[pid].z = flipRatio * (vp.z + flip_dw) + (1.0f - flipRatio) * pic_w;
}

void cudaParticleSimulator::g2p()
{
    int nx = grid_size.x, ny = grid_size.y, nz = grid_size.z;

    int blockSize = 256;
    int gridDim = (numParticles + blockSize - 1) / blockSize;

    g2pKernel<<<gridDim, blockSize>>>(
        deviceData.pos, deviceData.vel, deviceData.old_vel,
        grid_data.u, grid_data.v, grid_data.w,
        grid_data.u_old, grid_data.v_old, grid_data.w_old,
        nx, ny, nz,
        1.0f / grid_size.cellSize,
        fluidProps.flipRatio,
        numParticles);

    cudaDeviceSynchronize();
}
__global__ void advectionKernel(float3 *pos, float3 *vel,
    int numParticles, float deltaTime, float3 gridMin, float3 gridMax)
{
    int pid = blockIdx.x * blockDim.x + threadIdx.x;
    if (pid >= numParticles) return;

    pos[pid].x += vel[pid].x * deltaTime;
    pos[pid].y += vel[pid].y * deltaTime;
    pos[pid].z += vel[pid].z * deltaTime;

    // Rimbalzo sui bordi
    if (pos[pid].x < gridMin.x) { pos[pid].x = gridMin.x; vel[pid].x *= -0.5f; }
    if (pos[pid].x > gridMax.x) { pos[pid].x = gridMax.x; vel[pid].x *= -0.5f; }
    if (pos[pid].y < gridMin.y) { pos[pid].y = gridMin.y; vel[pid].y *= -0.5f; }
    if (pos[pid].y > gridMax.y) { pos[pid].y = gridMax.y; vel[pid].y *= -0.5f; }
    if (pos[pid].z < gridMin.z) { pos[pid].z = gridMin.z; vel[pid].z *= -0.5f; }
    if (pos[pid].z > gridMax.z) { pos[pid].z = gridMax.z; vel[pid].z *= -0.5f; }
}

void cudaParticleSimulator::computeAdvection()
{
    int blockSize = 256;
    int gridDim = (numParticles + blockSize - 1) / blockSize;
    float3 gridMin = {0.0f, 0.0f, 0.0f};
    float3 gridMax = {grid_size.x * grid_size.cellSize,
                      grid_size.y * grid_size.cellSize,
                      grid_size.z * grid_size.cellSize};
    advectionKernel<<<gridDim, blockSize>>>(
        deviceData.pos, deviceData.vel,
        numParticles, this->deltaTime, gridMin, gridMax);

    cudaDeviceSynchronize();
}