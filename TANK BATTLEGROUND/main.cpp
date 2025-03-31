#include "game.h"

int main(int argc, char* argv[]) {
    Game game;

    // Khởi tạo game với tiêu đề và kích thước cửa sổ
    if (!game.Initialize("TANKS BATTLEGROUND", 800, 800)) {
        return -1; // Thoát nếu khởi tạo thất bại
    }

    // Bắt đầu vòng lặp game chính
    game.Run();

    return 0;
}
