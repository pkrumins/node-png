class FbConverter {
    int bpp, depth,
        red_max, green_max, blue_max,
        red_shift, green_shift, blue_shift;

public:
    FbConverter(int bbpp, int ddepth,
        int rred_max, int ggreen_max, int bblue_max,
        int rred_shift, int ggreen_shift, int bblue_shift) :
        bpp(bbpp), depth(ddepth),
        red_max(rred_max), green_max(ggreen_max), blue_max(bblue_max),
        red_shift(rred_shift), green_shift(ggreen_max), blue_shift(bblue_shift) {}

    FbConverter() {}

    unsigned char *
    to_rgba(const unsigned char *input, int width, int height) {
        int depth_shift = 0;
        if (bpp != depth) depth_shift = bpp-depth;

        int new_length = 4 * width * height;
        unsigned char *output = (unsigned char *)
            malloc(sizeof(unsigned char) * new_length);
        if (!output) {
            ThrowException(Exception::Error(
                String::New("malloc failed in node-png (FbConverter::to_rgba).")
            ));
        }

        // couldn't yet figure out how to avoid all this copy/paste based on
        // bpp size.
        if (bpp == 8) {
            unsigned char val;
            for (int i = 0, j = 0; i < width*height*bpp/8; i+=bpp/8, j+=4) {
                val = (char)(input[i] >> depth_shift);
                output[j] = (val >> red_shift) & red_max;
                output[j+1] = (val >> green_shift) & green_max;
                output[j+2] = (val >> blue_shift) & blue_max;
                output[j+3] = 0xFF;
            }
        }
        else if (bpp == 16) {
            unsigned int val;
            for (int i = 0, j = 0; i < width*height*bpp/8; i+=bpp/8, j+=4) {
                val = (unsigned int)(input[i] >> depth_shift);
                output[j] = (val >> red_shift) & red_max;
                output[j+1] = (val >> green_shift) & green_max;
                output[j+2] = (val >> blue_shift) & blue_max;
                output[j+3] = 0xFF;
            }
        }
        else if (bpp == 32) {
            unsigned long val;
            for (int i = 0, j = 0; i < width*height*bpp/8; i+=bpp/8, j+=4) {
                val = (unsigned long)(input[i] >> depth_shift);
                output[j] = (val >> red_shift) & red_max;
                output[j+1] = (val >> green_shift) & green_max;
                output[j+2] = (val >> blue_shift) & blue_max;
                output[j+3] = depth == 32 ? val&0xFF : 0xFF;
            }

        }
        else {
            char message[100];
            snprintf(message, 100, "Unknown bpp size %d", bpp);
            ThrowException(Exception::Error(String::New(message)));
        }

        return output;
    }
};

