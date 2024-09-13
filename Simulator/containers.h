#ifndef math
#include <math.h>
#endif
#include <iostream>

/*
    Includes premade 2d Vector and RGB color classes
*/


class Vector2 {
    public:
        float x, y;

        // Both x and y are automatically converted to floats
        template <typename T1, typename T2> Vector2(T1 _x, T2 _y) : x((float) _x), y((float) _y) {}

        Vector2() : x(0.0), y(0.0) {};

        
        Vector2 operator +(const Vector2& vec) const
        {
            return Vector2(x + vec.x, y + vec.y);
        }

        Vector2 operator +() const
        {
            return Vector2(x, y);
        }

        Vector2 operator -(const Vector2& vec) const
        {
            return Vector2(x - vec.x, y - vec.y);
        }

        Vector2 operator -() const
        {
            return Vector2(-x, -y);
        }

        template<typename T> Vector2 operator *(T scalar) const
        {
            return Vector2(scalar*x, scalar*y);
        }

        // MUST be defined as friend, only way to make multiplication commutative (bc we need to overload both Vector2*(num) AND (num)*Vector2)
        template<typename T> friend Vector2 operator *(T scalar, const Vector2& vec)
        {
            return Vector2(scalar*vec.x, scalar*vec.y);
        }

        float operator *(const Vector2& vec) const
        {
            return this->dot(vec); 
        }

        template<typename T> Vector2 operator /(T scalar) const
        {
            return Vector2(x/scalar, y/scalar);
        }

        Vector2& operator +=(const Vector2& vec)
        {
            x += vec.x;
            y += vec.y;

            return *this;
        }

        Vector2& operator -=(const Vector2& vec)
        {
            x -= vec.x;
            y -= vec.y;
            return *this;
        }

        bool operator ==(const Vector2& vec) const
        {
            return (x == vec.x && y == vec.y);
        }

        template<typename T> Vector2& operator *=(T scalar)
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        template<typename T> Vector2& operator /=(T scalar)
        {
            x /= scalar;
            y /= scalar;
            return *this;
        }

        // returns a new vector, currently there's no in-place version
        Vector2 normalized()
        {
            return (*this)/getLength();
        }

        float dot(const Vector2& vec) const
        {
            return x*vec.x + y*vec.y;
        }

        float angleTo(const Vector2& vec) const // Outputs Degrees
        {
            return acos((x*vec.x + y*vec.y)/(getLength()*vec.getLength()))*180/M_PI;        
        }

        float getLength() const
        {
            return sqrt(x*x + y*y);
        }

        float getLengthSquared() const
        {
            return x*x + y*y;
        }

        float getDistanceTo(const Vector2& vec) const
        {
            Vector2 diff = vec - *this;

            return diff.getLength();
        }

        float getDistanceSquaredTo(const Vector2& vec) const
        {
            return (vec - (*this)).getLengthSquared();
        }

};

class RGBColor{
    public:
        int r, g, b, a;

        RGBColor(int R, int G, int B, int A = 255) : r(R), g(G), b(B), a(A) {}
};