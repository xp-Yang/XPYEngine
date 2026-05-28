#ifndef Rect_hpp
#define Rect_hpp

struct IntRect {
    IntRect() = default;
    IntRect(int _x, int _y, int _width, int _height)
        : x(_x)
        , y(_y)
        , width(_width)
        , height(_height)
    {}

    bool contains(float px, float py) const
    {
        return x <= px && px <= x + width &&
               y <= py && py <= y + height;
    }

    float aspectRatio() const
    {
        return height != 0 ? (float)width / (float)height : 1.0f;
    }

    int x{ 0 };
    int y{ 0 };
    int width{ 0 };
    int height{ 0 };
};

#endif
