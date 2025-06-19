#ifndef PERLIN3D_H
#define PERLIN3D_H

#include "random.h"
#include "math_util.h"
#include <cmath>

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

class Perlin3D {
  public:
    Perlin3D(float f, float d) : frequency(f), depth(d)
    {
        permutation = generate_perm(perm_count);

        perm = new int[perm_count*2];

        for(int i = 0; i < perm_count*2; i++)
            perm[i] = permutation[i%256];
    }

    ~Perlin3D() {
        delete[] permutation;
        delete[] perm;
    }

    float noise(float x, float y, float z) const
    {
        x *= frequency;
        y *= frequency;
        z *= frequency;
        int xi = static_cast<int>(floor(x)) & 255;
        int yi = static_cast<int>(floor(y)) & 255;
        int zi = static_cast<int>(floor(z)) & 255;
        float xf = x - floor(x);
        float yf = y - floor(y);
        float zf = z - floor(z);
        float u = fade(xf);
        float v = fade(yf);
        float w = fade(zf);

        int aaa = perm[perm[perm[xi  ] + yi  ] + zi  ];
        int baa = perm[perm[perm[xi+1] + yi  ] + zi  ];
        int aba = perm[perm[perm[xi  ] + yi+1] + zi  ];
        int bba = perm[perm[perm[xi+1] + yi+1] + zi  ];
        int aab = perm[perm[perm[xi  ] + yi  ] + zi+1];
        int bab = perm[perm[perm[xi+1] + yi  ] + zi+1];
        int abb = perm[perm[perm[xi  ] + yi+1] + zi+1];
        int bbb = perm[perm[perm[xi+1] + yi+1] + zi+1];

        float x1, x2, y1, y2;

        x1 = lerp(grad(aaa, xf, yf, zf), grad(baa, xf-1, yf, zf), u);
        x2 = lerp(grad(aba, xf, yf-1, zf), grad(bba, xf-1, yf-1, zf), u);
        y1 = lerp(x1, x2, v);

        x1 = lerp(grad(aab, xf, yf, zf-1), grad(bab, xf-1, yf, zf-1), u);
        x2 = lerp(grad(abb, xf, yf-1, zf-1), grad(bbb, xf-1, yf-1, zf-1), u);
        y2 = lerp(x1, x2, v);

        float val = lerp(y1, y2, w);
        return val;
    }

    float octaves(float x, float y, float z) const
    {
        float accum = 0.0;
        float freq = 1.0;
        float weight = 1.0;
        float max_val = 0.0;

        for (int i = 0; i < depth; i++){
            accum += weight * noise(x * freq, y * freq, z * freq);
            weight *= 0.5;
            freq *= 2;
            max_val += weight;
        }
        return (accum / max_val + 1.0) / 2.0;
    }

  private:
    static const int perm_count = 256;
    int *permutation;
    int *perm;

    float frequency;
    float depth;

    float grad(int hash, float x, float y, float z) const
	{
		switch(hash & 0xF)
		{
			case 0x0: return  x + y;
			case 0x1: return -x + y;
			case 0x2: return  x - y;
			case 0x3: return -x - y;
			case 0x4: return  x + z;
			case 0x5: return -x + z;
			case 0x6: return  x - z;
			case 0x7: return -x - z;
			case 0x8: return  y + z;
			case 0x9: return -y + z;
			case 0xA: return  y - z;
			case 0xB: return -y - z;
			case 0xC: return  y + x;
			case 0xD: return -y + z;
			case 0xE: return  y - x;
			case 0xF: return -y - z;
			default: return 0; // never happens
		}
	}
};

#endif