#ifndef PERLIN2D_H
#define PERLIN2D_H

#include "random.h"
#include "math_util.h"
#include <cmath>

class Perlin2D {
  public:
    Perlin2D()
    {
        permutation = generate_perm(perm_count);

        perm = new int[perm_count*2];

        for(int i = 0; i < perm_count*2; i++)
            perm[i] = permutation[i%256];
    }

    ~Perlin2D() {
        delete[] permutation;
        delete[] perm;
    }

    float noise(float x, float y) const
    {
        int xi = static_cast<int>(floor(x)) & 255;
        int yi = static_cast<int>(floor(y)) & 255;
        float xf = x - floor(x);
        float yf = y - floor(y);
        float u = fade(xf);
        float v = fade(yf);

        int aa = perm[perm[xi  ] + yi  ];
        int ba = perm[perm[xi+1] + yi  ];
        int ab = perm[perm[xi  ] + yi+1];
        int bb = perm[perm[xi+1] + yi+1];

        float y1 = lerp(grad(aa, xf, yf  ), grad(ba, xf-1, yf  ), u);
        float y2 = lerp(grad(ab, xf, yf-1), grad(bb, xf-1, yf-1), u);
        return lerp(y1, y2, v);
    }

    float octaves(float x, float y, int depth) const
    {
        float accum = 0.0;
        float freq = 1.0;
        float weight = 1.0;
        float max_val = 0.0;

        for (int i = 0; i < depth; i++){
            accum += weight * noise(x * freq, y * freq);
            weight *= 0.5;
            freq *= 2;
            max_val += weight;
        }
        
        return norm(accum / max_val);
    }

  private:
    static const int perm_count = 256;
    int *permutation;
    int *perm;

    void permute(int* arr, int n)
    {
        for (int i = n-1; i > 0; i--)
        {
            int random_index = random_int(0, i);
            std::swap(arr[i], arr[random_index]);
        }
    }

    int* generate_perm(int n) {
        auto p = new int[n];

        for(int i = 0; i < n; i++)
            p[i] = i;

        permute(p, n);

        return p;
    }

    // Perform dot product of (x, y) with 4 random vectors i, j, -i, -j
    float grad(int hash, float x, float y) const
    {
        hash &= 3;
        if (hash == 0) return x;
        else if (hash == 1) return -x;
        else if (hash == 2) return y;
        else if (hash == 3) return -y;
        return 0;
    }
};

void getPerlinMap(float* grid, int width, int height, float frequency, int depth=1)
{
    float dx = frequency / width ;
    float dy = frequency / height;

    float x = 0.0f, y = 0.0f;

    Perlin2D perlin_perm = Perlin2D();

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++) {
            grid[j * width + i] = (depth==1) ? norm(perlin_perm.noise(x, y)) : perlin_perm.octaves(x, y, depth);
            x += dx;
        }
        y += dy;
        x = 0.0f;
    }
}

#endif