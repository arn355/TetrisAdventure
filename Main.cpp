#include <chrono>
#include <random>
#include <iostream>
#include <time.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "Headers/DrawText.hpp"
#include "Headers/Global.hpp"
#include "Headers/GetTetromino.hpp"
#include "Headers/GetWallKickData.hpp"
#include "Headers/Tetromino.hpp"

int main()
{
	//Random seed
	srand(time(0));
	//Used to check whether the game is over or not
	bool game_over = 0;
	//Is the hard drop button pressed?
	bool hard_drop_pressed = 0;
	//Is the rotate button pressed?
	bool rotate_pressed = 0;

	//Used to make the game framerate-independent
	unsigned lag = 0;
	//How many lines the player cleared?
	unsigned lines_cleared = 0;

	//Timer for the line clearing effect
	unsigned char clear_effect_timer = 0;
	//Initial fall speed!
	unsigned char current_fall_speed = START_FALL_SPEED;
	//Fall speed = start fall speed / current fall speed
	unsigned char fall_speed = START_FALL_SPEED/current_fall_speed;
	//Timer for the tetromino falling
	unsigned char fall_timer = 0;
	//Timer for the tetromino moving horizontally
	unsigned char move_timer = 0;
	//Next shape (The shape that comes after the current shape)
	unsigned char next_shape;
	//Timer for the tetromino's soft drop
	unsigned char soft_drop_timer = 0;
	//Surrender key
	unsigned char surrender = 0;
	//Fever Mode
	unsigned char feverMode = 0;

	//Level
	unsigned int level = 1;

	//Line limit & Line left
	int lineLimit = (level * 2) + 1;
	int lineLeft = lineLimit;

	//Time limit & Time left
	int timeLimit = 60 + (level*20) ;
	int timeLeft = timeLimit;

	//Fever count & Fever Left
	int feverCount = 0;
	int feverLeft = 0;

	//Background color (FEVER TIME??)
	int bg = 10;

	//Random number for SPECIAL EVENT!!
	int lineEvent = rand() % 5 + 7;
	//int lineEvent = 5;

	//Similar to lag, used to make the game framerate-independent
	std::chrono::time_point<std::chrono::steady_clock> previous_time;

	//This is a random device!
	std::random_device random_device;

	//Random engine
	std::default_random_engine random_engine(random_device());

	//Distribution of all the shapes. We're gonna randomly choose one from them
	std::uniform_int_distribution<unsigned short> shape_distribution(0, 6);

	//Stores the current state of each row. Whether they need to be cleared or not
	std::vector<bool> clear_lines(ROWS, 0);

	//All the colors for the cells
	std::vector<sf::Color> cell_colors = {
		sf::Color(32, 32, 32), //Background
		sf::Color(33, 250, 250), //I tetromino
		sf::Color(51, 51, 255), //Reverse L tetromino
		sf::Color(255, 128, 0), //L tetromino
		sf::Color(233, 233, 40), //Square tetromino
		sf::Color(66, 236, 47), //Reverse Z tetromino
		sf::Color(181, 40, 242), //T tetromino
		sf::Color(226, 33, 33), //Z tetromino
		sf::Color(96, 96, 96), //Ghost tetromino
		sf::Color(255, 153, 204), //FEVER TIME!!
		sf::Color(255, 255, 255) //White Effect!
	};

	//Game matrix. Everything will happen to this matrix
	std::vector<std::vector<unsigned char>> matrix(COLUMNS, std::vector<unsigned char>(ROWS));

	//Music
	sf::Music music;
	if (!music.openFromFile("sounds/Tetris_theme.ogg"))
	{
		std::cout << "ERROR" << std::endl;
	}
	music.setVolume(20.f);
	music.setLoop(true);
	music.play();

	//Sound
	sf::SoundBuffer clearLineSound;
	if (!clearLineSound.loadFromFile("sounds/line.wav"))
	{
		std::cout << "ERROR" << std::endl;
	}
	sf::SoundBuffer fallSound;
	if (!fallSound.loadFromFile("sounds/fall.wav"))
	{
		std::cout << "ERROR" << std::endl;
	}
	sf::Sound sound;
	sound.setVolume(50.f);
	

	//Time countdown
	sf::Clock countdown;
	sf::Time timeCountdown;
	countdown.restart();

	//Fever time countdown
	sf::Clock feverClock;
	sf::Time feverTime;
	feverClock.restart();

	//Stores events
	sf::Event event;

	//Window
	sf::RenderWindow window(sf::VideoMode(2 * CELL_SIZE * COLUMNS * SCREEN_RESIZE, CELL_SIZE * ROWS * SCREEN_RESIZE), "Tetris Adventure", sf::Style::Close);
	//Resizing the window
	window.setView(sf::View(sf::FloatRect(0, 0, 2 * CELL_SIZE * COLUMNS, CELL_SIZE * ROWS)));

	//Falling tetromino. At the start we're gonna give it a random shape
	Tetromino tetromino(static_cast<unsigned char>(shape_distribution(random_engine)), matrix);

	//Generate a random shape and store it as the next shape
	next_shape = static_cast<unsigned char>(shape_distribution(random_engine));

	//Get the current time and store it in the variable
	previous_time = std::chrono::steady_clock::now();

	//While the window is open
	while (1 == window.isOpen())
	{
		//Get the difference in time between the current frame and the previous frame
		unsigned delta_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - previous_time).count();

		//Add the difference to the lag
		lag += delta_time;

		//In other words, we're updating the current time for the next frame.
		previous_time += std::chrono::microseconds(delta_time);

		timeCountdown = countdown.getElapsedTime();
		int timePass = (int)timeCountdown.asSeconds();
		if (timePass / 1 == 1)
		{
			timeLeft -= 1;
			countdown.restart();	
		}

		feverTime = feverClock.getElapsedTime();
		int feverPass = (int)feverTime.asSeconds();
		if (feverPass / 1 == 1)
		{
			feverLeft += 1;
			feverClock.restart();
			//std::cout << feverLeft << std::endl;
		}
		

		//While the lag exceeds the maximum allowed frame duration
		while (FRAME_DURATION <= lag)
		{
			//Subtract the right thing from the left thing
			lag -= FRAME_DURATION;

			//Looping through the events
			while (1 == window.pollEvent(event))
			{
				//Check the event type
				switch (event.type)
				{
					//If the user closed the game
				case sf::Event::Closed:
				{
					//Close the window
					window.close();

					break;
				}
				//If the key has been released
				case sf::Event::KeyReleased:
				{
					//Check which key was released
					switch (event.key.code)
					{
						//If it's Up
					case sf::Keyboard::Up:
					{
						//Rotation key is not pressed anymore
						rotate_pressed = 0;

						break;
					}
					//If it's Down
					case sf::Keyboard::Down:
					{
						//Reset the soft drop timer
						soft_drop_timer = 0;

						break;
					}
					//If it's Left or Right
					case sf::Keyboard::Left:
					case sf::Keyboard::Right:
					{
						//Reset the move timer
						move_timer = 0;

						break;
					}
					//If it's Space
					case sf::Keyboard::Space:
					{
						//Hard drop key is not pressed anymore
						hard_drop_pressed = 0;
					}
					}
				}
				}
			}

			//If the clear effect timer is 0
			if (0 == clear_effect_timer)
			{
				//If the game over is 0
				if (0 == game_over)
				{
					//If the rotate pressed is 0
					if (0 == rotate_pressed)
					{
						//If the Up is pressed
						if (1 == sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
						{
							//Rotation key is pressed!
							rotate_pressed = 1;

							//Do a barrel roll
							tetromino.rotate(1, matrix);
						}
					}

					//If the move timer is 0
					if (0 == move_timer)
					{
						//If the Left is pressed
						if (1 == sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
						{
							//Reset the move timer
							move_timer = 1;

							//Move the tetromino to the left
							tetromino.move_left(matrix);
						}
						else if (1 == sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
						{
							//Reset the move timer
							move_timer = 1;

							//Move the tetromino to the Right
							tetromino.move_right(matrix);
						}
					}
					else
					{
						//Update the move timer
						move_timer = (1 + move_timer) % MOVE_SPEED;
					}

					//If hard drop is not pressed
					if (0 == hard_drop_pressed)
					{
						//But the Space is pressed, which is the hard drop key
						if (1 == sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
						{
							//PREEEEESSED!!
							hard_drop_pressed = 1;

							//Reset the fall timer
							fall_timer = current_fall_speed;

							//Make the falling tetromino drop HAAAAARD!
							tetromino.hard_drop(matrix);

							//fall sound
							sound.setBuffer(fallSound);
							sound.play();
						}
					}
					//If surrender is not pressed
					if (0 == surrender)
					{
						//If S is pressed
						if (1 == sf::Keyboard::isKeyPressed(sf::Keyboard::S))
						{
							//Surrender
							surrender = 1;
							//Gameover
							game_over = 1;
						}
					}

					//If soft drop not pressed
					if (0 == soft_drop_timer)
					{
						if (1 == sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
						{
							if (1 == tetromino.move_down(matrix))
							{
								fall_timer = 0;
								soft_drop_timer = 1;
							}
						}
					}
					else
					{
						soft_drop_timer = (1 + soft_drop_timer) % SOFT_DROP_SPEED;
					}

					//If the fall timer is over
					if (current_fall_speed == fall_timer)
					{
						//If the tetromino can't move down anymore
						if (0 == tetromino.move_down(matrix))
						{
							//Put the falling tetromino to the matrix
							tetromino.update_matrix(matrix);

							//Loop through every row
							for (unsigned char a = 0; a < ROWS; a++)
							{
								//Here we're gonna check if the current row should be cleared or not
								bool clear_line = 1;

								//Check if the every cell in the row is filled or not
								for (unsigned char b = 0; b < COLUMNS; b++)
								{
									if (0 == matrix[b][a])
									{
										clear_line = 0;

										break;
									}
								}

								//If we have to clear it
								if (1 == clear_line)
								{
									//WE CLEAR IT!
									//First we increase the score
									//Then we decrease the lineLeft
									//Then we increase the feverCount
									if (feverMode == 1)
									{
										lines_cleared += 2;
										lineLeft -= 2;
										feverCount++;
									}
									else
									{
										lines_cleared++;
										lineLeft--;
										feverCount++;
									}
									
									//Then we start the effect timer
									clear_effect_timer = CLEAR_EFFECT_DURATION;

									//Set the current row as the row that should be cleared
									clear_lines[a] = 1;

									//If the player reached a certain number of lines
									if (0 == lines_cleared % LINES_TO_INCREASE_SPEED)
									{
										//We increase the game speed
										current_fall_speed = std::max<unsigned char>(SOFT_DROP_SPEED, current_fall_speed - 1);
									}
								}
								if (lineLeft <= 0)
								{
									level++;
									lineLimit = (level * 2) + 1;
									lineLeft = lineLimit;
									timeLimit = 60 + (level * 20);
									timeLeft = timeLimit;
								}
								if (feverCount >= lineEvent)
								{
									int randomEvent = rand() % 2;
									if (randomEvent == 0)
									{
										feverMode = 1;
										feverLeft = 0;
										feverClock.restart();
									}
									else if (randomEvent == 1)
									{
										timeLeft += 30;
									}
									feverCount = 0;
									lineEvent = rand() % 5 + 7;
									//lineEvent = 5;
								}
							}

							//If the effect timer is over
							if (0 == clear_effect_timer)
							{
								//Decide if the game is over or not based on the return value of the reset function
								game_over = 0 == tetromino.reset(next_shape, matrix);

								//Generate the next shape
								next_shape = static_cast<unsigned char>(shape_distribution(random_engine));
							}
						}

						//Restart the fall timer
						fall_timer = 0;
					}
					else
					{
						//Increment the fall timer
						fall_timer++;
					}
				} //This is the code for restarting the game
				else if (1 == sf::Keyboard::isKeyPressed(sf::Keyboard::Enter) && 1 == surrender)
				{
					//We set everything to 0
					game_over = 0;
					surrender = 0;
					hard_drop_pressed = 0;
					rotate_pressed = 0;

					level = 1;
					lineLimit = (level * 2) + 1;
					lineLeft = lineLimit;

					lines_cleared = 0;

					//Except the current fall speed because he's a special boy
					current_fall_speed = START_FALL_SPEED;
					fall_timer = 0;
					move_timer = 0;
					soft_drop_timer = 0;

					timeLimit = 60 + (level*20);
					timeLeft = timeLimit;
					countdown.restart();

					//Then we clear the matrix
					for (std::vector<unsigned char>& a : matrix)
					{
						std::fill(a.begin(), a.end(), 0);
					}
				}
				else if (1 == game_over && 0 == surrender)
				{
					//We set everything to 0
					game_over = 0;
					surrender = 0;
					hard_drop_pressed = 0;
					rotate_pressed = 0;

					//Except the current fall speed
					current_fall_speed = START_FALL_SPEED;
					fall_timer = 0;
					move_timer = 0;
					soft_drop_timer = 0;

					timeLeft -= 30;	
					

					//Then we clear the matrix
					for (std::vector<unsigned char>& a : matrix)
					{
						std::fill(a.begin(), a.end(), 0);
					}
				}
			}
			else
			{
				//Decrement the effect timer
				clear_effect_timer--;

				//If the effect timer is between 1 and -1
				if (0 == clear_effect_timer)
				{
					//Clear line sound
					sound.setBuffer(clearLineSound);
					sound.play();
					//Loop through each row
					for (unsigned char a = 0; a < ROWS; a++)
					{
						//If the row should be cleared
						if (1 == clear_lines[a])
						{
							//Loop through each cell in the row
							for (unsigned char b = 0; b < COLUMNS; b++)
							{
								//Set the cell to 0 (empty) (the absence of existence)
								matrix[b][a] = 0;

								//Swap the row with the rows above
								for (unsigned char c = a; 0 < c; c--)
								{
									matrix[b][c] = matrix[b][c - 1];
									matrix[b][c - 1] = 0;
								}
							}
						}
					}

					game_over = 0 == tetromino.reset(next_shape, matrix);

					next_shape = static_cast<unsigned char>(shape_distribution(random_engine));

					//Clear the clear lines array
					std::fill(clear_lines.begin(), clear_lines.end(), 0);
				}
			}

			//Here we're drawing everything!
			if (FRAME_DURATION > lag)
			{
				//Calculating the size of the effect squares
				unsigned char clear_cell_size = static_cast<unsigned char>(2 * round(0.5f * CELL_SIZE * (clear_effect_timer / static_cast<float>(CLEAR_EFFECT_DURATION))));

				//We're gonna use this object to draw every cell in the game
				sf::RectangleShape cell(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));
				//Next shape preview border (White square at the corner)
				sf::RectangleShape preview_border(sf::Vector2f(5 * CELL_SIZE, 5 * CELL_SIZE));
				preview_border.setFillColor(sf::Color(32, 32, 32));
				preview_border.setOutlineThickness(-1);
				preview_border.setPosition(CELL_SIZE * (1.5f * COLUMNS - 2.5f), CELL_SIZE * (0.25f * ROWS - 2.5f));

				//Clear the window from the previous frame
				window.clear();

				//Draw the matrix
				for (unsigned char a = 0; a < COLUMNS; a++)
				{
					for (unsigned char b = 0; b < ROWS; b++)
					{
						if (0 == clear_lines[b])
						{
							cell.setPosition(static_cast<float>(CELL_SIZE * a), static_cast<float>(CELL_SIZE * b));

							if (1 == game_over && 0 < matrix[a][b] && 1 == surrender)
							{
								cell.setFillColor(cell_colors[8]);
							}
							else
							{
								cell.setFillColor(cell_colors[matrix[a][b]]);
							}

							window.draw(cell);
						}
					}
				}

				//Set the cell color to gray (ghost tetromino)
				cell.setFillColor(cell_colors[8]);

				//FEVER TIMEEEEEEEEE!!
				if (feverMode == 1)
				{
					if (feverLeft <= 10)
					{
						bg = 9;
						//std::cout << feverLeft << std::endl;
					}
					else
					{
						feverMode = 0;
						feverLeft = 0;
						bg = 10;
					}
				}

				//If time out
				if (timeLeft <= 0)
				{
					game_over = 1;
					surrender = 1;	
				}

				//If the game is not over
				if (0 == game_over)
				{
					//Drawing the ghost tetromino
					for (Position& mino : tetromino.get_ghost_minos(matrix))
					{
						cell.setPosition(static_cast<float>(CELL_SIZE * mino.x), static_cast<float>(CELL_SIZE * mino.y));

						window.draw(cell);
					}

					cell.setFillColor(cell_colors[1 + tetromino.get_shape()]);
				}

				//Drawing the falling tetromino
				for (Position& mino : tetromino.get_minos())
				{
					cell.setPosition(static_cast<float>(CELL_SIZE * mino.x), static_cast<float>(CELL_SIZE * mino.y));

					window.draw(cell);
				}

				//Drawing the effect
				for (unsigned char a = 0; a < COLUMNS; a++)
				{
					for (unsigned char b = 0; b < ROWS; b++)
					{
						if (1 == clear_lines[b])
						{
							cell.setFillColor(cell_colors[0]);
							cell.setPosition(static_cast<float>(CELL_SIZE * a), static_cast<float>(CELL_SIZE * b));
							cell.setSize(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));

							window.draw(cell);

							cell.setFillColor(cell_colors[bg]);
							cell.setPosition(floor(CELL_SIZE * (0.5f + a) - 0.5f * clear_cell_size), floor(CELL_SIZE * (0.5f + b) - 0.5f * clear_cell_size));
							cell.setSize(sf::Vector2f(clear_cell_size, clear_cell_size));

							window.draw(cell);
						}
					}
				}

				//Draw the preview tetromino
				cell.setFillColor(cell_colors[1 + next_shape]);
				cell.setSize(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));

				//Draw the preview border
				window.draw(preview_border);

				//Draw the next tetromino
				for (Position& mino : get_tetromino(next_shape, static_cast<unsigned char>(1.5f* COLUMNS), static_cast<unsigned char>(0.25f* ROWS)))
				{
					//Shifting the tetromino to the center of the preview border
					unsigned short next_tetromino_x = CELL_SIZE * mino.x;
					unsigned short next_tetromino_y = CELL_SIZE * mino.y;

					if (0 == next_shape)
					{
						next_tetromino_y += static_cast<unsigned char>(round(0.5f * CELL_SIZE));
					}
					else if (3 != next_shape)
					{
						next_tetromino_x -= static_cast<unsigned char>(round(0.5f * CELL_SIZE));
					}

					cell.setPosition(next_tetromino_x, next_tetromino_y);

					window.draw(cell);
				}


				//calculate all score
				if (1 == game_over && 1 == surrender)
				{
					fall_speed = 0;
					timeLeft = 0;
					lineLeft = lines_cleared;
				}
				else
				{
					fall_speed = START_FALL_SPEED / current_fall_speed;
				}

				//Drawing the text
				draw_text(static_cast<unsigned short>(CELL_SIZE * (0.5f + COLUMNS)), static_cast<unsigned short>(0.5f * CELL_SIZE * ROWS), "Level:" + std::to_string(level) + "\nLines:" + std::to_string(lineLeft) + "\nSpeed:" + std::to_string(fall_speed) + 'x' + "\nTime:" + std::to_string(timeLeft), window);
				
				//DISPLAY!
				window.display();

			}
		}
	}
}