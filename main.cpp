#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

struct YUVFrame {
    vector<uint8_t> Y;
    vector<uint8_t> U;
    vector<uint8_t> V;
    int width;
    int height;
};

// Функция для чтения YUV420
YUVFrame readYUVFrame(ifstream &file, int width, int height) {
    YUVFrame frame;
    frame.width = width;
    frame.height = height;
    int y_size = width * height;
    int uv_size = (width / 2) * (height / 2);

    frame.Y.resize(y_size);
    frame.U.resize(uv_size);
    frame.V.resize(uv_size);

    file.read(reinterpret_cast<char *>(frame.Y.data()), y_size);
    file.read(reinterpret_cast<char *>(frame.U.data()), uv_size);
    file.read(reinterpret_cast<char *>(frame.V.data()), uv_size);

    return frame;
}

// Функция для записи YUV420
void writeYUVFrame(ofstream &file, const YUVFrame &frame) {
    file.write(reinterpret_cast<const char *>(frame.Y.data()), frame.width * frame.height);
    file.write(reinterpret_cast<const char *>(frame.U.data()), (frame.width / 2) * (frame.height / 2));
    file.write(reinterpret_cast<const char *>(frame.V.data()), (frame.width / 2) * (frame.height / 2));
}

// Структура для BMP файла (заголовок и данные)
struct BMPImage {
    int width;
    int height;
    vector<uint8_t> data; // Данные изображения в формате RGB
};

// Чтение BMP файла
BMPImage readBMP(const string &filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        throw runtime_error("Unable to open BMP file.");
    }

    // Чтение заголовка BMP (54 байта)
    uint8_t header[54];
    file.read(reinterpret_cast<char *>(header), 54);

    // Извлечение размеров изображения
    int width = *reinterpret_cast<int *>(&header[18]);
    int height = *reinterpret_cast<int *>(&header[22]);

    // BMP хранит данные с построчной выравниваемостью на 4 байта
    int row_padded = (width * 3 + 3) & (~3);
    vector<uint8_t> data(row_padded * height);
    file.read(reinterpret_cast<char *>(data.data()), row_padded * height);

    // Инвертируем строки изображения (переворачиваем по вертикали)
    vector<uint8_t> flipped_data(row_padded * height);
    for (int i = 0; i < height; ++i) {
        copy(data.begin() + i * row_padded, 
             data.begin() + (i + 1) * row_padded, 
             flipped_data.begin() + (height - 1 - i) * row_padded);
    }

    BMPImage image{width, height, flipped_data};
    return image;
}

// Преобразование изображения из RGB в YUV420
YUVFrame convertRGBToYUV420(const BMPImage &bmp) {
    int width = bmp.width;
    int height = bmp.height;
    
    YUVFrame frame;
    frame.width = width;
    frame.height = height;

    frame.Y.resize(width * height);
    frame.U.resize((width / 2) * (height / 2));
    frame.V.resize((width / 2) * (height / 2));

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int rgb_index = (j * width + i) * 3;
            uint8_t r = bmp.data[rgb_index + 2]; // BMP хранит цвета в порядке BGR
            uint8_t g = bmp.data[rgb_index + 1];
            uint8_t b = bmp.data[rgb_index + 0];

            // Преобразование в YUV (параметры взяты из формул)
            uint8_t y = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
            uint8_t u = static_cast<uint8_t>(-0.169 * r - 0.331 * g + 0.5 * b + 128);
            uint8_t v = static_cast<uint8_t>(0.5 * r - 0.419 * g - 0.081 * b + 128);

            frame.Y[j * width + i] = y;
            if (i % 2 == 0 && j % 2 == 0) {
                int uv_index = (j / 2) * (width / 2) + (i / 2);
                frame.U[uv_index] = u;
                frame.V[uv_index] = v;
            }
        }
    }

    return frame;
}

// Наложение картинки на центр каждого кадра YUV420
void overlayYUVFrame(YUVFrame &frame, const YUVFrame &overlay) {
    int x_offset = (frame.width - overlay.width) / 2;
    int y_offset = (frame.height - overlay.height) / 2;

    for (int j = 0; j < overlay.height; ++j) {
        for (int i = 0; i < overlay.width; ++i) {
            // Накладываем Y компонент
            frame.Y[(j + y_offset) * frame.width + (i + x_offset)] = overlay.Y[j * overlay.width + i];

            // Накладываем U и V компоненты только для чётных строк и столбцов
            if (j % 2 == 0 && i % 2 == 0) {
                int uv_frame_index = ((j + y_offset) / 2) * (frame.width / 2) + (i + x_offset) / 2;
                int uv_overlay_index = (j / 2) * (overlay.width / 2) + (i / 2);

                frame.U[uv_frame_index] = overlay.U[uv_overlay_index];
                frame.V[uv_frame_index] = overlay.V[uv_overlay_index];
            }
        }
    }
}

int main() {
    // Открываем видеоряд и изображение
    ifstream video_file("park_joy_1080p50.yuv", ios::binary);
    ofstream output_file("output_video.yuv", ios::binary);

    BMPImage bmp = readBMP("car_512x512.bmp");

    // Настройки входящего видеоряда
    int width = 1920;
    int height = 1080;
    int num_frames = 500;

    // Функция преобразования
    YUVFrame overlay_frame = convertRGBToYUV420(bmp);

    for (int i = 0; i < num_frames; ++i) {
        YUVFrame frame = readYUVFrame(video_file, width, height);
        overlayYUVFrame(frame, overlay_frame);
        writeYUVFrame(output_file, frame);
    }

    video_file.close();
    output_file.close();

    cout << "Processing complete!" << endl;
    return 0;
}
