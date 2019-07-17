#include<iostream>
#include<algorithm>
#include<utility>
#include <vector>
#include <queue>
#include <map>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ros/ros.h>
#include <octomap/octomap.h>
#include <fstream>
#include <istream>
#include <list>
#include <octomap/ColorOcTree.h>
#include <octomap/OcTree.h>
#include <octomap/OcTreeBase.h>
#include <octomap/AbstractOcTree.h>
#include <octomap/OcTreeIterator.hxx>
#include<octomap/AbstractOccupancyOcTree.h>
#include <time.h>

using namespace std;
using namespace octomap;
using namespace octomath;

//3D point structure
struct Point
{
  int x, y, z;
};

//Point structure including cost
struct PointCost
{
  int x, y, z;
  float cost;
};
//Keeps track of future movements from current node
struct Front
{
  float cost;
  Point pos;
  Point previous;
};

class myComparator
{
public:
    int operator() (const Front& cost1, const Front& cost2)
    {
        return cost1.cost > cost2.cost;
    }
};

//Create Dijkstra's algorithm
vector<Point> Dijkstras(Point start, Point goal, OcTree* octo) {
   clock_t t;
   t=clock();
  //Initialize to first node which has cost 0, it's only option is start, and it came from no other nodes
  //Front front[?] = {{.00001, start, NULL}};
  int min_x = -36;
  int max_x = 36;
  int min_y = -36;
  int max_y = 36;
  int min_z = 0;
  int max_z = 72;
  priority_queue <Front, vector<Front>, myComparator > front;
  front.push({.00001, start, {0,0,0}});
  //Initialize visited array to be the size of all of the nodes Initialized all to zeroes
  int env_x = abs(min_x)+max_x;
  int env_y = abs(min_y)+max_y;
  int env_z = abs(min_z)+max_z;
  double visited[env_x][env_y][env_z] = {0};
  //Create an array that keeps track of what each point came from
  Point came_from[env_x][env_y][env_z];
  //Initialize the possible movements which contain (x,y,z) points and the cost to move to those points
  PointCost movements[6] = {{1,0,0,1},{0,1,0,1},{-1,0,0,1},{0,-1,0,1},{0,0,1,1},{0,0,-1,1}};
  Point pos_of_node;
  //Find the node with the minimum cost (not using a heap)
  while(front.size()>0){
/*
    cout << "What is in the front?" << endl;
    for(int i = 0; i < front.size(); i++){
	cout << front.size() << endl;
    }
*/
    //remove the node with the minimum cost
    Front min_node = front.top();
    front.pop();


    //Find the cost, position, and previous node of the current node and store those values
    float cost_of_node = min_node.cost;
    pos_of_node = min_node.pos;
    Point previous_node = min_node.previous;

    //Check if this node has been visited before
    int x_array = pos_of_node.x;
    if(pos_of_node.x < 0){
    	x_array = abs(pos_of_node.x)+max_x-1;
    }
    int y_array = pos_of_node.y;
    if(pos_of_node.y < 0){
    	y_array = abs(pos_of_node.y)+max_y-1;
    }

    if (visited[x_array][y_array][pos_of_node.z] > 0){
      continue;
    }
    //if it was not visited, it now is so add it to the visited array

    visited[x_array][y_array][pos_of_node.z] = cost_of_node;
    //update the came_from array to show the previous node that is associated with this node
    came_from[x_array][y_array][pos_of_node.z] = previous_node;
    //Are we at the goal?
    if (pos_of_node.x == goal.x && pos_of_node.y == goal.y && pos_of_node.z == goal.z) {
      break;
    }
    //Coordinates of new current node
    int new_x = 0;
    int new_y = 0;
    int new_z = 0;
    //Checking the neighbors
    for(int i = 0; i < 6; i++){
      new_x=pos_of_node.x+movements[i].x;
      new_y=pos_of_node.y+movements[i].y;
      new_z=pos_of_node.z+movements[i].z;
      if(new_x<min_x || new_x >= max_x || new_y<min_y || new_y>=max_y || new_z<min_z || new_z>=max_z){
        continue;
      }
      Point new_pos;
      new_pos.x = new_x;
      new_pos.y = new_y;
      new_pos.z = new_z;
      int new_x_array = new_pos.x;
      if(new_pos.x < 0){
      	new_x_array = abs(new_pos.x)+max_x-1;
      }
      int new_y_array = new_pos.y;
      if(new_pos.y < 0){
    	  new_y_array = abs(new_pos.y)+max_y-1;
      }
      double temp_x = new_pos.x;
      double temp_y = new_pos.y;
      double temp_z = new_pos.z;
      OcTreeNode* node = octo->search(temp_x, temp_y, temp_z, 0);
      int occupied = 0;

      if(node == 0 && visited[new_x_array][new_y_array][new_pos.z]==0 ){
	for(OcTree::leaf_bbx_iterator it = octo -> begin_leafs_bbx(Vector3 (new_pos.x-0.8,new_pos.y-0.8, new_pos.z-0.8), Vector3 (new_pos.x+ 0.8,new_pos.y+0.8, new_pos.z+0.8)), end = octo -> end_leafs_bbx(); it != end; ++it){
                occupied = octo ->isNodeOccupied(*it);
 		if(occupied == 1){
		  break;
		}

        }
	if(occupied == 0){
	   front.push({cost_of_node+movements[i].cost, new_pos, pos_of_node});

        }

       }
      else if(visited[x_array][y_array][new_pos.z]==0 && octo->isNodeOccupied(node) ==0){
	for(OcTree::leaf_bbx_iterator it = octo -> begin_leafs_bbx(Vector3 (new_pos.x-0.8,new_pos.y-0.8, new_pos.z-0.8), Vector3 (new_pos.x+ 0.8,new_pos.y+0.8, new_pos.z+0.8)), end = octo -> end_leafs_bbx(); it != end; ++it){
                occupied = octo ->isNodeOccupied(*it);
 		if(occupied == 1){
	          break;
		}
        }
	if(occupied == 0){
	   front.push({cost_of_node+movements[i].cost, new_pos, pos_of_node});

        }
      }
    }
  }
  //cout << "Break the loop" << endl;
  //Initialize optimal_path array which contains (x,y,z) points
  vector<Point> path;
  if(pos_of_node.x == goal.x && pos_of_node.y == goal.y && pos_of_node.z == goal.z){
    //cout << "In the if statement" << endl;
    while(pos_of_node.x != start.x || pos_of_node.y != start.y || pos_of_node.z != start.z){
      int x_array = pos_of_node.x;
      if(pos_of_node.x < 0){
    	  x_array = abs(pos_of_node.x)+max_x-1;
      }
      int y_array = pos_of_node.y;
      if(pos_of_node.y < 0){
    	  y_array = abs(pos_of_node.y)+max_y-1;
      }
      path.push_back(pos_of_node);
      //cout << "adding: " << pos_of_node.x << ", " << pos_of_node.y << ", " << pos_of_node.z << endl;
      pos_of_node = came_from[x_array][y_array][pos_of_node.z];
    }
    path.push_back(start);
    reverse(path.begin(), path.end());
  } else {
       cout << "Your goal is unreachable" << endl;
    }
    t = clock()-t;
    cout <<"This took : " << (float)t/CLOCKS_PER_SEC << " seconds" << endl;
  return path;
}

int main(){
  Point start = {0,0,1};
  Point goal = {5,0,1};
  AbstractOcTree* tree = AbstractOcTree::read("obstaclecourse2.ot");
  OcTree* bt = dynamic_cast<OcTree*>(tree);
  //in python code they create an array of all zeros that will contain a value of 255 if an obstacle is there
  //world_obstacles = np.zeros(world_extents, dtype=np.uint8)
  //int obstacles_in_env[env_x][env_y][env_z] = {0};
  //If there is an obstacle at a node, set that node to be 255
  //STILL NEED TO CODE WHEN THE TIME COMES

  vector<Point> results;
  results = Dijkstras(start, goal, bt);
  cout << "Size of Results : "<< results.size() << endl;
  cout << "The Goals is ( " << goal.x << " ," <<goal.y<< " ,"<<goal.z <<" )"<< endl; 
  ofstream outfile ("Waypoints_DJ.txt");
  for (int i = 1; i < results.size(); i ++){
  outfile << "0 " << results[i].x << " " << results[i].y << " " << results[i].z << " " << "0" << endl;
  }
outfile.close();
  //Use waypoint script information to actually utilize the Points in our vector to move from point to point

}