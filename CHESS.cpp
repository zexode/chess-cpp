#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cctype>
#include <sstream>

using namespace std;

class Piece {
public:
    string color;
    bool has_moved = false;
    virtual string symbol() const = 0;
    virtual vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) = 0;
    virtual ~Piece() {}
};  // end of ChessGame

class Checker : public Piece {
public:
    bool is_king = false;

    string symbol() const override {
        if (is_king) return (color == "white") ? "WK" : "BK";
        return (color == "white") ? "W" : "B";
    }

    vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) override {
        vector<pair<int, int>> moves;
        int i = pos.first, j = pos.second;
        int dir = (color == "white") ? -1 : 1;
        vector<pair<int, int>> directions = is_king ?
            vector<pair<int, int>>{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}} :
            vector<pair<int, int>>{{dir, 1}, {dir, -1}};

        for (auto [dr, dc] : directions) {
            int ni = i + dr, nj = j + dc;
            if (ni >= 0 && ni < 8 && nj >= 0 && nj < 8 && board[ni][nj] == nullptr) {
                moves.push_back({ni, nj});
            }
            int jump_r = i + 2 * dr, jump_c = j + 2 * dc;
            if (ni >= 0 && ni < 8 && nj >= 0 && nj < 8 &&
                jump_r >= 0 && jump_r < 8 && jump_c >= 0 && jump_c < 8) {
                auto mid = board[ni][nj];
                if (mid && mid->color != color && board[jump_r][jump_c] == nullptr) {
                    moves.push_back({jump_r, jump_c});
                }
            }
        }
        return moves;
    }
};


class Game {
public:
    vector<vector<shared_ptr<Piece>>> board;
    vector<vector<vector<shared_ptr<Piece>>>> move_history;

    void save_state() {
        vector<vector<shared_ptr<Piece>>> snapshot(8, vector<shared_ptr<Piece>>(8, nullptr));
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 8; ++j)
                if (board[i][j]) snapshot[i][j] = board[i][j];  // поверхностная копия
        move_history.push_back(snapshot);
    }

    void undo_moves(int n) {
        if (n <= move_history.size()) {
            board = move_history[move_history.size() - n];
            move_history.resize(move_history.size() - n);
            cout << "Вернулись на " << n << " ход(ов) назад." << endl;
        } else {
            cout << "Невозможно вернуть столько ходов назад." << endl;
        }
    }

    virtual void play() = 0;
    virtual ~Game() {}
};



class CheckersGame : public Game {
public:
    string turn = "white";

    CheckersGame() {
        board = vector<vector<shared_ptr<Piece>>>(8, vector<shared_ptr<Piece>>(8, nullptr));
        init_board();
    }

    void init_board() {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 8; ++j) {
                if ((i + j) % 2 == 1) {
                    board[i][j] = make_shared<Checker>();
                    board[i][j]->color = "black";
                }
            }
        }
        for (int i = 5; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                if ((i + j) % 2 == 1) {
                    board[i][j] = make_shared<Checker>();
                    board[i][j]->color = "white";
                }
            }
        }
    }

    // метод play добавлен
    void play() override {
        while (true) {
            string start_str, end_str;
            cout << "Введите ход (например, c3 e5): ";
            cin >> start_str >> end_str;
            int sr = 8 - (start_str[1] - '0');
            int sc = tolower(start_str[0]) - 'a';
            int er = 8 - (end_str[1] - '0');
            int ec = tolower(end_str[0]) - 'a';
            if (sr < 0 || sr >= 8 || sc < 0 || sc >= 8 || er < 0 || er >= 8 || ec < 0 || ec >= 8) {
                cout << "Некорректные координаты\n";
                continue;
            }
            auto piece = board[sr][sc];
            if (!piece) {
                cout << "Нет фигуры в начальной позиции\n";
                continue;
            }
            // Выполнение хода
            board[er][ec] = piece;
            board[sr][sc] = nullptr;

            auto checker = dynamic_pointer_cast<Checker>(piece);
            if (checker && !checker->is_king &&
                ((checker->color == "white" && er == 0) || (checker->color == "black" && er == 7))) {
                checker->is_king = true;
                cout << (checker->color == "white" ? "Белая" : "Чёрная") << " шашка стала дамкой!\n";
            }

            while (true) {
                auto captures = checker->get_possible_moves({er, ec}, board);
                vector<pair<int, int>> capture_moves;
                for (auto &m : captures) {
                    if (abs(m.first - er) == 2) capture_moves.push_back(m);
                }
                if (capture_moves.empty()) break;

                cout << "Дополнительное взятие возможно: ";
                for (auto &m : capture_moves) {
                    cout << "(" << char('a' + m.second) << 8 - m.first << ") ";
                }
                cout << "\nВведите следующий ход или 'skip': ";
                string next;
                cin >> next;
                if (next == "skip") break;

                if (next.size() != 2) {
                    cout << "Неверный формат хода\n"; break;
                }

                int ner = 8 - (next[1] - '0');
                int nec = tolower(next[0]) - 'a';
                if (find(capture_moves.begin(), capture_moves.end(), make_pair(ner, nec)) == capture_moves.end()) {
                    cout << "Неверный ход для взятия\n"; break;
                }

                int mr = (er + ner) / 2, mc = (ec + nec) / 2;
                board[mr][mc] = nullptr;
                board[ner][nec] = checker;
                board[er][ec] = nullptr;
                er = ner; ec = nec;

                if (!checker->is_king &&
                    ((checker->color == "white" && er == 0) || (checker->color == "black" && er == 7))) {
                    checker->is_king = true;
                    cout << (checker->color == "white" ? "Белая" : "Чёрная") << " шашка стала дамкой!\n";
                    break;
                }
            }

            turn = (turn == "white") ? "black" : "white";
        }
    }
};

// Пример класса пешки. Остальные фигуры и классы (Rook, King и т.д.) будут аналогично добавлены.
class Pawn : public Piece {
public:
    string symbol() const override {
        return (color == "white") ? "P" : "p";
    }

    vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) override {
        vector<pair<int, int>> moves;
        int i = pos.first, j = pos.second;
        int direction = (color == "white") ? -1 : 1;

        if (i + direction >= 0 && i + direction < 8 && board[i + direction][j] == nullptr) {
            moves.push_back({i + direction, j});
            if (!has_moved && board[i + 2 * direction][j] == nullptr) {
                moves.push_back({i + 2 * direction, j});
            }
        }

        for (int dj : {-1, 1}) {
            int ni = i + direction;
            int nj = j + dj;
            if (ni >= 0 && ni < 8 && nj >= 0 && nj < 8) {
                if (board[ni][nj] && board[ni][nj]->color != color) {
                    moves.push_back({ni, nj});
                }
            }
        }

        return moves;
    }
};

class Rook : public Piece {
public:
    string symbol() const override {
        return (color == "white") ? "R" : "r";
    }

    vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) override {
        vector<pair<int, int>> moves;
        int i = pos.first, j = pos.second;
        vector<pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (auto [di, dj] : directions) {
            int rr = i + di, cc = j + dj;
            while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
                if (board[rr][cc] == nullptr) {
                    moves.push_back({rr, cc});
                } else {
                    if (board[rr][cc]->color != color) {
                        moves.push_back({rr, cc});
                    }
                    break;
                }
                rr += di;
                cc += dj;
            }
        }
        return moves;
    }
};

class Knight : public Piece {
public:
    string symbol() const override {
        return (color == "white") ? "N" : "n";
    }

    vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) override {
        vector<pair<int, int>> moves;
        int i = pos.first, j = pos.second;
        vector<pair<int, int>> offsets = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1},
                                           {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};
        for (auto [di, dj] : offsets) {
            int rr = i + di;
            int cc = j + dj;
            if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
                if (!board[rr][cc] || board[rr][cc]->color != color) {
                    moves.push_back({rr, cc});
                }
            }
        }
        return moves;
    }
};

class Bishop : public Piece {
public:
    string symbol() const override {
        return (color == "white") ? "B" : "b";
    }

    vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) override {
        vector<pair<int, int>> moves;
        int i = pos.first, j = pos.second;
        vector<pair<int, int>> directions = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (auto [di, dj] : directions) {
            int rr = i + di, cc = j + dj;
            while (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
                if (board[rr][cc] == nullptr) {
                    moves.push_back({rr, cc});
                } else {
                    if (board[rr][cc]->color != color) {
                        moves.push_back({rr, cc});
                    }
                    break;
                }
                rr += di;
                cc += dj;
            }
        }
        return moves;
    }
};

class Queen : public Piece {
public:
    string symbol() const override {
        return (color == "white") ? "Q" : "q";
    }

    vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) override {
        Rook temp_rook;
        temp_rook.color = this->color;
        Bishop temp_bishop;
        temp_bishop.color = this->color;
        auto rook_moves = temp_rook.get_possible_moves(pos, board);
        auto bishop_moves = temp_bishop.get_possible_moves(pos, board);
        rook_moves.insert(rook_moves.end(), bishop_moves.begin(), bishop_moves.end());
        return rook_moves;
    }
};

class King : public Piece {
public:
    string symbol() const override {
        return (color == "white") ? "K" : "k";
    }

    vector<pair<int, int>> get_possible_moves(pair<int, int> pos, vector<vector<shared_ptr<Piece>>> &board) override {
        vector<pair<int, int>> moves;
        int i = pos.first, j = pos.second;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int rr = i + dr, cc = j + dc;
                if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
                    if (!board[rr][cc] || board[rr][cc]->color != color) {
                        moves.push_back({rr, cc});
                    }
                }
            }
        }
        return moves;
    }
};


class ChessGame : public Game {
public:
    string turn = "white";
    pair<int, int> en_passant_target = {-1, -1};

    ChessGame() {
        board = vector<vector<shared_ptr<Piece>>>(8, vector<shared_ptr<Piece>>(8, nullptr));
        init_board();
    }

    void init_board() {
        for (int j = 0; j < 8; ++j) {
            board[1][j] = make_shared<Pawn>();
            board[1][j]->color = "black";
            board[6][j] = make_shared<Pawn>();
            board[6][j]->color = "white";
        }
        board[0][0] = make_shared<Rook>(); board[0][0]->color = "black";
        board[0][1] = make_shared<Knight>(); board[0][1]->color = "black";
        board[0][2] = make_shared<Bishop>(); board[0][2]->color = "black";
        board[0][3] = make_shared<Queen>(); board[0][3]->color = "black";
        board[0][4] = make_shared<King>(); board[0][4]->color = "black";
        board[0][5] = make_shared<Bishop>(); board[0][5]->color = "black";
        board[0][6] = make_shared<Knight>(); board[0][6]->color = "black";
        board[0][7] = make_shared<Rook>(); board[0][7]->color = "black";

        board[7][0] = make_shared<Rook>(); board[7][0]->color = "white";
        board[7][1] = make_shared<Knight>(); board[7][1]->color = "white";
        board[7][2] = make_shared<Bishop>(); board[7][2]->color = "white";
        board[7][3] = make_shared<Queen>(); board[7][3]->color = "white";
        board[7][4] = make_shared<King>(); board[7][4]->color = "white";
        board[7][5] = make_shared<Bishop>(); board[7][5]->color = "white";
        board[7][6] = make_shared<Knight>(); board[7][6]->color = "white";
        board[7][7] = make_shared<Rook>(); board[7][7]->color = "white";
    }

    bool is_under_threat(pair<int, int> pos, string color_check) {
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                auto piece = board[i][j];
                if (piece && piece->color != color_check) {
                    auto moves = piece->get_possible_moves({i, j}, board);
                    if (find(moves.begin(), moves.end(), pos) != moves.end()) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void play() override {
        while (true) {
            // Отображение доски
            cout << "  A B C D E F G H\n";
            for (int i = 0; i < 8; ++i) {
                cout << 8 - i << " ";
                for (int j = 0; j < 8; ++j) {
                    if (board[i][j]) {
                        string sym = board[i][j]->symbol();
                        if (is_under_threat({i, j}, board[i][j]->color)) sym += "!";
                        cout << sym << ' ';
                    } else {
                        cout << ". ";
                    }
                }
                cout << 8 - i << "\n";
            }
            cout << "  A B C D E F G H\n";

            cout << turn << " ход. Введите ход (например, e2 e4) или 'назад n': ";
            string move_input;
            getline(cin >> ws, move_input);

            if (move_input.starts_with("назад")) {
                istringstream iss(move_input);
                string cmd;
                int n;
                iss >> cmd >> n;
                if (iss && n > 0) {
                    undo_moves(n);
                    continue;
                } else {
                    cout << "Неверная команда 'назад'\n";
                    continue;
                }
            }

            istringstream iss(move_input);
            string start_str, end_str;
            iss >> start_str >> end_str;
            if (start_str.empty() || end_str.empty()) {
                cout << "Неверный формат хода\n";
                continue;
            }

            int start_row = 8 - (start_str[1] - '0');
            int start_col = tolower(start_str[0]) - 'a';
            int end_row = 8 - (end_str[1] - '0');
            int end_col = tolower(end_str[0]) - 'a';

            if (start_row < 0 || start_row >= 8 || start_col < 0 || start_col >= 8 ||
                end_row < 0 || end_row >= 8 || end_col < 0 || end_col >= 8) {
                cout << "Некорректные координаты\n";
                continue;
            }

            auto piece = board[start_row][start_col];
            if (!piece || piece->color != turn) {
                cout << "На выбранной клетке нет вашей фигуры\n";
                continue;
            }

            auto moves = piece->get_possible_moves({start_row, start_col}, board);
            if (find(moves.begin(), moves.end(), make_pair(end_row, end_col)) == moves.end()) {
                cout << "Недопустимый ход\n";
                continue;
            }

            save_state();

            // Обработка взятия на проходе
            if (auto p = dynamic_pointer_cast<Pawn>(piece)) {
                if (make_pair(end_row, end_col) == en_passant_target) {
                    int captured_row = (turn == "white") ? end_row + 1 : end_row - 1;
                    board[captured_row][end_col] = nullptr;
                    cout << "Взятие на проходе выполнено!\n";
                }
            }

            // Выполнение хода
            board[end_row][end_col] = piece;
            board[start_row][start_col] = nullptr;

            // Проверка двойного хода пешки
            if (auto p = dynamic_pointer_cast<Pawn>(piece)) {
                if (abs(end_row - start_row) == 2) {
                    en_passant_target = {(start_row + end_row) / 2, start_col};
                } else {
                    en_passant_target = {-1, -1};
                }

                // Превращение в ферзя
                if ((p->color == "white" && end_row == 0) || (p->color == "black" && end_row == 7)) {
                    board[end_row][end_col] = make_shared<Queen>();
                    board[end_row][end_col]->color = p->color;
                    cout << "Пешка превращается в ферзя!\n";
                }
            } else {
                en_passant_target = {-1, -1};
            }

            piece->has_moved = true;
            turn = (turn == "white") ? "black" : "white";

            // Проверка на шах и мат
            pair<int, int> king_pos = {-1, -1};
            for (int i = 0; i < 8; ++i) {
                for (int j = 0; j < 8; ++j) {
                    auto k = dynamic_pointer_cast<King>(board[i][j]);
                    if (k && k->color == turn) {
                        king_pos = {i, j};
                        break;
                    }
                }
            }

            if (king_pos.first != -1 && is_under_threat(king_pos, turn)) {
                bool has_escape = false;
                auto king = board[king_pos.first][king_pos.second];
                auto king_moves = king->get_possible_moves(king_pos, board);
                for (auto [i, j] : king_moves) {
                    auto temp = board[i][j];
                    board[i][j] = king;
                    board[king_pos.first][king_pos.second] = nullptr;
                    if (!is_under_threat({i, j}, turn)) {
                        has_escape = true;
                    }
                    board[king_pos.first][king_pos.second] = king;
                    board[i][j] = temp;
                    if (has_escape) break;
                }
                if (has_escape) {
                    cout << "Шах!\n";
                } else {
                    cout << "Мат! Игра окончена.\n";
                    return;
                }
            }
        }
    }
};

int main() {
    cout << "Во что вы хотите сыграть? (шахматы/шашки): ";
    string choice;
    getline(cin, choice);

    if (choice.find("шах") != string::npos) {
        ChessGame game;
        game.play();
    } else if (choice.find("шаш") != string::npos) {
        CheckersGame game;
        game.play();
    } else {
        cout << "Неверный выбор. Завершение программы.\n";
    }

    return 0;
}
