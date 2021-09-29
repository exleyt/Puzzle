#include <SFML/Graphics.hpp>
#include <iostream>
#include <deque>
#include <unordered_map>

using namespace sf;
using namespace std;

// Change in current index to move in the direction
#define LEFT -1
#define RIGHT 1
#define UP -4
#define DOWN 4
#define NONE 0

#define FOUND 0
#define NOT_FOUND -1

// A possible state of the board
typedef struct node
{
    char board[16];
    string hash;
    char blank_index;
    
    // The direction the blank tile moved to reach this state
    char move;
} node;


const int board_length = 4;
const int board_size = 16;
char board[board_size] = { 0 };

const int blank_value = 0;
int blank_index = 0;
// Position of blank tile on the window
Vector2i blank(0, 0);

const int tile_size = 56;
const int tile_offset = 6;
const int sprite_count = 15;
Sprite tile_sprites[sprite_count];

// Movement limiter will only allow tile to be moved along blank tile's row or column
Vector2i ml(0, 0);

// Path from current state to goal state
deque<node*> path;

// Returns a string hash of the board
string getHash(char* board)
{
    string hash = "";
    for (int i = 0; i < board_size; i++)
        hash += board[i];
    return hash;
}

// Copys the state of the src board to dest board
void copyBoard(char* dest, char* src)
{
    for (int i = 0; i < board_size; i++)
        dest[i] = src[i];
}

// Returns a new node where the given move was applied to prev_n
node* createNode(node* prev_n, char move)
{
    node* n = new struct node;
    copyBoard(n->board, prev_n->board);
    n->blank_index = prev_n->blank_index + move;
    n->board[prev_n->blank_index] = n->board[n->blank_index];
    n->board[n->blank_index] = blank_value;
    n->hash = getHash(n->board);
    n->move = move;
    return n;
}

// Returns a vector of possible nodes* that don't repeat the previous move
vector<node*> successors(node* prev_n)
{
    vector<node*> moves;

    if (prev_n->blank_index % board_length != 0 && prev_n->move != RIGHT)
        moves.push_back(createNode(prev_n, LEFT));
    if (prev_n->blank_index % board_length != 3 && prev_n->move != LEFT)
        moves.push_back(createNode(prev_n, RIGHT));
    if (prev_n->blank_index / board_length != 0 && prev_n->move != DOWN)
        moves.push_back(createNode(prev_n, UP));
    if (prev_n->blank_index / board_length != 3 && prev_n->move != UP)
        moves.push_back(createNode(prev_n, DOWN));

    return moves;
}

// Returns manhattan distance of the board to goal state
int manhattanH(char* board)
{
    int total_distance = 0;
    for (int i = 0; i < board_size; i++)
        if (board[i] != blank_value)
            total_distance += abs((board[i] - 1 - i) / board_length) + abs((board[i] - 1) % board_length - i % board_length);
    return total_distance;
}

// Returns true if the path contains the current node
bool contains(deque<node*>& path, node* v)
{
    for (node* u : path)
    {
        if (u->hash == v->hash)
            return true;
    }
    return false;
}

// IDA* recursive search
int search(deque<node*>& path, int g, int bound)
{
    node* n = path.back();
    int f = g + manhattanH(n->board);
    if (f > bound)
        return f;
    // manhattanH is zero when f == g meaning we are at goal state 
    if (f == g)
        return FOUND;
    int min = INT_MAX;
        
    for (node* succ : successors(n))
    {
        if (!contains(path, succ))
        {
            path.push_back(succ);
            int t = search(path, g + 1, bound);
            if (t == FOUND)
                return FOUND;
            if (t < min)
                min = t;
            delete path.back();
            path.pop_back();
        }
        else
            delete succ;
    }
    return min;
}

// IDA* main control loop, returns nubmer of moves needed to reach goal state
int idaStar()
{
    node* root = new struct node;
    copyBoard(root->board, board);
    root->blank_index = blank_index;
    root->hash = getHash(root->board);
    root->move = NONE;

    int bound = manhattanH(root->board);

    // Cleans path of last search
    if (!path.empty())
    {
        for (node* u : path)
            delete u;

        path.clear();
    }

    path.push_back(root);

    do
    {
        int t = search(path, 0, bound);
        if (t == FOUND)
            return bound;
        if (t == NOT_FOUND)
            return NOT_FOUND;
        bound = t;
    } while (true);
}

// Updates location of blank tile
void placeBlank()
{
    blank.x = tile_offset + blank_index % board_length * tile_size;
    blank.y = tile_offset + blank_index / board_length * tile_size;
}

// Updates location of all tiles
void placeTiles()
{
    for (int i = 0; i < board_size; i++)
    {
        if (board[i] == blank_value)
        {
            blank_index = i;
            placeBlank();
        }
        else
        {
            tile_sprites[board[i] - 1].setPosition(tile_offset + i % board_length * tile_size
                , tile_offset + i / board_length * tile_size);
        }
    }
}

// Returns wether or not the current state can reach the goal state
bool isSolvable()
{
    int inversions = 0;

    for (int i = 0; i < board_size - 1; i++)
        for (int j = i + 1; j < board_size; j++)
            if (board[i] > board[j] && i != blank_index && j != blank_index)
                inversions++;

    return inversions % 2 != (blank_index / board_length) % 2;
}

// Replaces the last two nubmer tiles making the board solvable
void makeSolvable()
{
    int x1;
    int x2;

    for (int i = 0; i < board_size; i++)
    {
        if (board[i] == sprite_count)
            x1 = i;
        else if (board[i] == sprite_count - 1)
            x2 = i;
    }

    tile_sprites[sprite_count - 1].setPosition(tile_offset + x2 % board_length * tile_size
        , tile_offset + x2 / board_length * tile_size);
    tile_sprites[sprite_count - 2].setPosition(tile_offset + x1 % board_length * tile_size
        , tile_offset + x1 / board_length * tile_size);

    board[x1] = sprite_count - 1;
    board[x2] = sprite_count;
}

// Randomly places all of the tiles into a solvable state
void buildRandomBoard()
{
    for (int i = 0; i < board_size; i++)
        board[i] = 0;

    // Randomly place tile_sprites
    for (int i = 0; i < sprite_count; i++)
    {
        int r = rand() % board_size;
        int r_dir = (rand() % 2 == 0) ? -1 : 1;

        do
        {
            if (board[r] == 0)
            {
                board[r] = i + 1;
                break;
            }
            else if (r_dir == 1 && r == board_size - 1)
                r = 0;
            else if (r_dir == -1 && r == 0)
                r = board_size - 1;
            else
                r += r_dir;
        } while (true);
    }

    placeTiles();

    if (!isSolvable())
        makeSolvable();
}

// Checks if the current state is the goal state
bool isSolution(char* board)
{
    for (int i = 0; i < board_size - 1; i++)
        if (board[i] != i + 1)
            return false;
    return true;
}

// Updates ml to have the direction from the n tile to the blank tile if adjacent
Vector2i getMovementLimiter(int n)
{
    Vector2i delta_position(0, 0);

    // if n is left of blank
    if (blank_index % board_length != 0 && board[blank_index - 1] == n)
        delta_position.x++;
    // if n is right of blank
    else if (blank_index % board_length != 3 && board[blank_index + 1] == n)
        delta_position.x--;
    // if n is below blank
    else if (blank_index / board_length != 0 && board[blank_index - board_length] == n)
        delta_position.y++;
    // if n is above blank
    else if (blank_index / board_length != 3 && board[blank_index + board_length] == n)
        delta_position.y--;

    return delta_position;
}

int main()
{
    srand(time(NULL));

    RenderWindow window(VideoMode(236, 236), "15-Puzzle", Style::Titlebar | Style::Close);
    window.setFramerateLimit(60);

    Texture t1, t2;
    t1.loadFromFile("res/board.png");
    t2.loadFromFile("res/tileset.png");

    Sprite board_sprite(t1);

    // Load tiles from tileset
    for (int i = 0; i < sprite_count; i++)
    {
        tile_sprites[i].setTexture(t2);
        tile_sprites[i].setTextureRect(IntRect(tile_size * i, 0, tile_size, tile_size));
    }

    buildRandomBoard();

    // Index of selected tile on board
    int tile_index;
    // Index of selected tile in tile_sprites
    int tsi;
    // Location of mouse on tile
    int x_offset, y_offset;
    // True when a tile is selected
    bool is_move = false;
    // True after IDA* while tiles are animating
    bool auto_animation = false;
    // True after IDA* while tiles are animating and moving
    bool moving = false;
    // Current board state in path while auto_animation
    node* current_node= NULL;
    // Index of current_node in path
    int current_index;
    // Location on window to move to 
    Vector2i move_to;

    while (window.isOpen())
    {
        Vector2i position = Mouse::getPosition(window);
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();

            // Updates that tile has been picked up
            if (event.type == Event::MouseButtonPressed && !auto_animation)
                if (event.key.code == Mouse::Left)
                    for (int i = 0; i < sprite_count; i++)
                        if (tile_sprites[i].getGlobalBounds().contains(position.x, position.y))
                        {
                            ml = getMovementLimiter(i + 1);
                            // If tile is adjacent to blank
                            if (ml.x != 0 || ml.y != 0)
                            {
                                is_move = true;
                                tile_index = blank_index - ml.x - board_length * ml.y;
                                tsi = i;
                                x_offset = position.x - tile_sprites[i].getPosition().x;
                                y_offset = position.y - tile_sprites[i].getPosition().y;
                            }
                        }

            if (event.type == Event::MouseButtonReleased && !auto_animation)
                if (event.key.code == Mouse::Left && is_move)
                {
                    // Snap forward and swap with blank
                    int blank_x_c = blank.x + tile_size / 2;
                    int blank_y_c = blank.y + tile_size / 2;
                    if (tile_sprites[tsi].getGlobalBounds().contains(blank_x_c, blank_y_c))
                    {
                        tile_sprites[tsi].setPosition(blank.x, blank.y);
                        board[blank_index] = tsi + 1;
                        board[tile_index] = blank_value;
                        int temp = blank_index;
                        blank_index = tile_index;
                        tile_index = temp;
                        placeBlank();

                        if (isSolution(board))
                            cout << "Solved!" << endl;
                        else
                            cout << "Not Solved!" << endl;
                    }
                    // Snap back to old position
                    else
                    {
                        int old_x = tile_offset + tile_index % board_length * tile_size;
                        int old_y = tile_offset + tile_index / board_length * tile_size;
                        tile_sprites[tsi].setPosition(old_x, old_y);
                    }
                    is_move = false;
                }

            if (event.type == Event::KeyReleased && !auto_animation)
                // Load nearly complete puzzle with last 6 tiles random (not scalable)
                if (event.key.code == Keyboard::L)
                {
                    is_move = false;

                    for (int i = 0; i < board_size; i++)
                        board[i] = 0;

                    board[1] = 2;
                    board[2] = 3;
                    board[3] = 4;
                    board[5] = 6;
                    board[6] = 7;
                    board[7] = 8;
                    for (int i = 0; i < board_length; i++)
                        board[4 * i] = 4 * i + 1;

                    // Randomly place last 6 tiles
                    char unused[6] = { 10,11,12,14,15 };
                    for (int i = 0; i < 6; i++)
                    {
                        int r = rand() % 6;
                        int r_dir = (rand() % 2 == 0) ? -1 : 1;

                        do
                        {
                            int j = (r % 3 + 1) + (r / 3 + 2) * 4;
                            if (board[j] == 0)
                            {
                                board[j] = unused[i];
                                break;
                            }
                            else if (r_dir == 1 && r == 5)
                                r = 0;
                            else if (r_dir == -1 && r == 0)
                                r = 5;
                            else
                                r += r_dir;
                        } while (true);
                    }

                    placeTiles();

                    if (!isSolvable())
                        makeSolvable();
                }
                // Do IDA* search for path to goal and then set flags for animating
                else if (event.key.code == Keyboard::S)
                {
                    is_move = false;
                    placeTiles();
                    cout << "Goal Depth: " << idaStar() << endl;
                   
                    if (path.size() > 1)
                    {
                        current_index = 1;
                        current_node = path[current_index];
                        moving = false;
                        auto_animation = true;
                    }
                }
                // Randomly build a new board
                else if (event.key.code == Keyboard::R)
                {
                    is_move = false;

                    buildRandomBoard();
                }
        }

        // For doing the animation
        if (auto_animation)
        {
            // Sets moving and updates the location to move_to
            if (!moving)
            {
                switch (current_node->move)
                {
                case LEFT:
                    tile_index = blank_index + LEFT;
                    break;
                case RIGHT:
                    tile_index = blank_index + RIGHT;
                    break;
                case UP:
                    tile_index = blank_index + UP;
                    break;
                case DOWN:
                    tile_index = blank_index + DOWN;
                    break;
                }
                move_to.x = tile_offset + blank_index % board_length * tile_size;
                move_to.y = tile_offset + blank_index / board_length * tile_size;
                moving = true;
            }
            else
            {
                Vector2f curr_pos = tile_sprites[board[tile_index] - 1].getPosition();
                // True if the current move is done
                if ((int) curr_pos.x == move_to.x && (int) curr_pos.y == move_to.y)
                {
                    // Snaps and updates positions on board and window
                    moving = false;
                    current_index++;
                    copyBoard(board, current_node->board);
                    placeTiles();

                    // Loads next node
                    if (current_index < path.size())
                        current_node = path[current_index];
                    // Animation is over
                    else
                    {
                        if (isSolution(board))
                            cout << "Solved!" << endl;
                        else
                            cout << "Not Solved!" << endl;
                        auto_animation = false;
                    }   
                }
                // Continue animating current move takes ~0.5s total
                else
                {
                    int x = curr_pos.x;
                    int y = curr_pos.y;
                    switch (current_node->move)
                    {
                    case LEFT:
                        x+=2;
                        break;
                    case RIGHT:
                        x-=2;
                        break;
                    case UP:
                        y+=2;
                        break;
                    case DOWN:
                        y-=2;
                        break;
                    }
                    tile_sprites[board[tile_index] - 1].setPosition(x, y);
                }
               
            }
        }

        // For moving selected tile
        if (is_move)
        {
            // Limits movement horizontally otherwise snaps towards mouse
            if (ml.x != 0)
            {
                int new_x = (position.x - x_offset);
                int old_x = tile_offset + tile_index % board_length * tile_size;
                int dx = new_x - old_x;
                int x = (dx != 0 && dx / abs(dx) == ml.x)
                    ? new_x : tile_sprites[tsi].getPosition().x;
                x = (x * ml.x > blank.x * ml.x) ? blank.x : x;
                tile_sprites[tsi].setPosition(x, tile_sprites[tsi].getPosition().y);
            }
            // Limits movement vertically otherwise snaps towards mouse
            else if (ml.y != 0)
            {
                int new_y = (position.y - y_offset);
                int old_y = tile_offset + tile_index / board_length * tile_size;
                int dy = new_y - old_y;
                int y = (dy != 0 && dy / abs(dy) == ml.y)
                    ? new_y : tile_sprites[tsi].getPosition().y;
                y = (y * ml.y > blank.y * ml.y) ? blank.y : y;
                tile_sprites[tsi].setPosition(tile_sprites[tsi].getPosition().x, y);
            }
        }

        // Update and draw window
        window.clear();
        window.draw(board_sprite);
        for (int i = 0; i < sprite_count; i++)
        {
            window.draw(tile_sprites[i]);
        }
        window.display();
    }

    return 0;
}