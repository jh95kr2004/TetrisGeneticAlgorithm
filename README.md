
# Tetris AI: Machine Learning using Genetic Algorithm
This project do machine learning using genetic algorithm to make Tetris AI.
## Getting started
These instructions will help you to let your computer learns playing Tetris.
### Tested Environment
 - Mac OS X 10.3
### Prerequisites
Your environment should have GCC compiler which supports OpenMP. Also ncurses library is required.
#### Mac OS X
If you are using Mac OS X, you have to install GCC 7 that can complie OpenMP. You can install by typing (You have to install Homebrew in advance.):
```
brew install gcc --without-multilib
```
### Installing
Just clone this git.
```
git clone https://github.com/jh95kr2004/TetrisGeneticAlgorithm.git
```
Now you have to compile the codes.
```
make
```
Then, main.out file will be created. If you want to remove object files and executable files, just type
```
make clean
```
## Run machine learning
### Run executable file
To run program, just type
```
./a.out
```
'a.out' is the executable file created by compiler. Now you can see the process of machine learning.
### Options
#### -t [number of threads]
This project supports multi-threading using OpenMP. If you want to use multi threads, use this option. For example, if you want to use 20 threads, type:
```
./a.out -t 20
```
#### -noscreen
By default, progress of the playing game will be printed on the screen. However, as print function is expensive, you can turn off printing the progress of the playing by typing this option to do machine learning much more faster.
### Output of terminal
As we are using genetic algorithm, there are concepts of generation, population and individual. Therefore, you can see the current order of generation and the learning progress of each individual  in the terminal. If you are using multiple threads by '-t' option, multiple individuals will play game simultaneously. Also, if you are not using '-noscreen' option, you can see the progress of playing game. Also, population of each generation is 20 and each individual plays game 20 times and get average score of them.
### Result of machine learning
You can get the result of machine learning in the 'output.txt' file. In this file, the weights of each generation which show the best performance will be written.
```
Gen 1 : -2.272263 -4.917595 -0.018812 1.177883 1.680085 2.191623 -4.612082 0.262576 
Gen 2 : -4.960563 -4.917595 -0.018812 1.040119 1.285719 4.085517 -2.844361 0.509945
```
For this example, the weights of the 1st and 2nd generations are written in the file. Each weights of generation is the weights of individual of corresponding generation that shows the best performance.
#### The meaning of each weight value:
 - Sum of heights of each column of field
 - The number of Holes
 - The number of Blockades
 - Score of reaching block to the floor
 - Score of removing lines
 - The number of of blocks reached at the wall
 - Standard deviation of heights of field
 - Difference between maximum height and minimum edge height of field
## How it works: Genetic Algorithm
Before read this paragraph, you can get the concept of genetic algorithm at this Wikipedia page: [Genetic Algorithm](https://en.wikipedia.org/wiki/Genetic_algorithm)<br>
### Selection
GA(Genetic algorithm) is inspired by the natural selection of breeding. Like as creatures are evolving as generations go by dominant entities are selected by nature, GA selects individuals of generation which show good performance to create next generation. For example, giraffes which have long neck are survived from which have short neck.
### Crossover
After selection, GA crosses the weights of selected individuals and creates new 20 individuals of next generation. Therefore, these new individuals have the mixed weights of dominant (good at playing Tetris) individuals of previous generation.
### Mutation
In nature, mutation is one of the most important things to be evolved. Likewise, in GA, mutation is occurred randomly every new generation is generated. In this project, the weights of individuals of new generation will be modified randomly in the range of -0.5 ~ 0.5.
### Fitness
GA uses fitness function to evaluate how much each individual shows good performance. In this project, playing game is fitness function. Therefore, every individual plays game automatically for 20 times and get the average of scores. Surely, better individual shows bigger average score.
## Application
Purpose of this project is obtaining the appropriate weights of factors for calculating score. You can use these weights to other Tetris projects to make other features.
## Authors
 - Junghoon Jang (jh95kr2003@gmail.com)
