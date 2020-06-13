#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"


Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

void viewport(int x, int y, int w, int h) {
    Viewport = Matrix::identity();
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 1.f;
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 0;
}

void projection(float coeff) {
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

void lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    Matrix Minv = Matrix::identity();
    Matrix Tr = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        Minv[0][i] = x[i];
        Minv[1][i] = y[i];
        Minv[2][i] = z[i];
        Tr[i][3] = -center[i];
    }
    ModelView = Minv * Tr;
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    float u[3];
    u[2] = (C[0] - A[0]) * (B[1] - A[1]) - (B[0] - A[0]) * (C[1] - A[1]);
    if (std::abs(u[2]) <= 1e-2) {
        return Vec3f(-1, 1, 1);
    } else {
        u[0] = (B[0] - A[0]) * (A[1] - P[1]) - (A[0] - P[0]) * (B[1] - A[1]);
    	u[1] = (A[0] - P[0]) * (C[1] - A[1]) - (C[0] - A[0]) * (A[1] - P[1]);
        return Vec3f(1.f - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]);
    }
}



void triangle(mat<4, 3, float> &clipc, IShader &shader, TGAImage &image, float *zbuffer) {
    mat<3, 4, float> pts = (Viewport * clipc).transpose(); // transposed to ease access to each of the points
    mat<3, 2, float> pts2;
    for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);

    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts2[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
        }
    }
    Vec2i P;
    TGAColor color;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);
            Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
            bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
            float frag_depth = clipc[2] * bc_clip;
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 ||
                zbuffer[P.x + P.y * image.get_width()] > frag_depth)
                continue;
            bool discard = shader.fragment(bc_clip, color);
            if (!discard) {
                zbuffer[P.x + P.y * image.get_width()] = frag_depth;
                image.set(P.x, P.y, color);
            }
        }
    }
}

