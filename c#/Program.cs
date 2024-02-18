using System;
using System.Collections.Generic;

namespace BabaSolver
{
	abstract class GameObject {}
	
	class Baba : GameObject
	{
		public int i;
		public int j;
		
		public Baba(int i, int j)
		{
			this.i = i;
			this.j = j;
		}
		
		public override string ToString()
		{
			return "B";
		}
	}
	
	class Rock : GameObject
	{
		public override string ToString()
		{
			return "R";
		}
	}
	
	class Door : GameObject
	{
		public override string ToString()
		{
			return "D";
		}
	}
	
	class Key : GameObject
	{
		public override string ToString()
		{
			return "K";
		}
	}
	
	class RockText : GameObject
	{
		public override string ToString()
		{
			return "1";
		}
	}
	
	class IsText : GameObject
	{
		public override string ToString()
		{
			return "2";
		}
	}
	
	class PushText : GameObject
	{
		public override string ToString()
		{
			return "3";
		}
	}
	
	class Tile : GameObject
	{
		public override string ToString()
		{
			return "^";
		}
	}
	
	class Immovable : GameObject
	{
		public override string ToString()
		{
			return "X";
		}
	}
	
	class GameState
	{
		public List<GameObject>[][] Grid;
		public List<Baba> Babas;
		
		public GameState(List<GameObject>[][] grid, List<Baba> babas)
		{
			this.Grid = grid;
			this.Babas = babas;
		}
	}
	
	class Program
	{
		static void Main(string[] args)
		{
			Console.WriteLine("Baba Is You solver");
			List<GameObject>[][] grid = new List<GameObject>[20][];
			for (int i = 0; i < grid.Length; ++i)
			{
				grid[i] = new List<GameObject>[20];
				for (int j = 0; j < grid[i].Length; ++j)
				{
					grid[i][j] = new List<GameObject>(4);
				}
			}
			
			// Add a boundary of immovables around the play area.
			for (int j = 0; j < grid[0].Length; ++j)
			{
				grid[0][j].Add(new Immovable());
			}
			for (int j = 0; j < grid[grid.Length-1].Length; ++j)
			{
				grid[grid.Length-1][j].Add(new Immovable());
			}
			for (int i = 1; i < grid.Length-1; ++i)
			{
				grid[i][0].Add(new Immovable());
				grid[i][grid[i].Length-1].Add(new Immovable());
			}
			// Add tiles to grid.
			for (int i = 4; i <= 8; ++i)
			{
				for (int j = 3; j <= 7; ++j)
				{
					grid[i][j].Add(new Tile());
				}
			}
			for (int i = 4; i <= 8; ++i)
			{
				for (int j = 11; j <= 15; ++j)
				{
					grid[i][j].Add(new Tile());
				}
			}
			for (int i = 11; i <= 15; ++i)
			{
				for (int j = 3; j <= 7; ++j)
				{
					grid[i][j].Add(new Tile());
				}
			}
			for (int i = 10; i <= 14; ++i)
			{
				for (int j = 11; j <= 15; ++j)
				{
					grid[i][j].Add(new Tile());
				}
			}
			// Add an immovable for each text block around the corners of the grid.
			grid[1][1].Add(new Immovable());
			grid[1][2].Add(new Immovable());
			grid[1][3].Add(new Immovable());
			grid[1][8].Add(new Immovable());
			grid[1][9].Add(new Immovable());
			grid[1][10].Add(new Immovable());
			grid[17][1].Add(new Immovable());
			grid[17][2].Add(new Immovable());
			grid[18][1].Add(new Immovable());
			grid[18][2].Add(new Immovable());
			grid[18][3].Add(new Immovable());
			grid[18][4].Add(new Immovable());
			grid[16][16].Add(new Immovable());
			grid[16][17].Add(new Immovable());
			grid[16][18].Add(new Immovable());
			grid[17][16].Add(new Immovable());
			grid[17][17].Add(new Immovable());
			grid[17][18].Add(new Immovable());
			grid[18][16].Add(new Immovable());
			grid[18][17].Add(new Immovable());
			grid[18][18].Add(new Immovable());
			// Add the other game objects to the grid.
			grid[5][4].Add(new Rock());
			grid[7][6].Add(new Rock());
			grid[7][12].Add(new Rock());
			grid[5][12].Add(new RockText());
			grid[5][13].Add(new IsText());
			grid[5][14].Add(new PushText());
			grid[17][3].Add(new PushText());
			grid[13][5].Add(new Door());
			grid[12][13].Add(new Key());
			List<Baba> babas = new List<Baba>(2);
			Baba baba1 = new Baba(6, 5);
			babas.Add(baba1);
			grid[baba1.i][baba1.j].Add(baba1);
			Baba baba2 = new Baba(6, 13);
			babas.Add(baba2);
			grid[baba2.i][baba2.j].Add(baba2);
			
			for (int i = 0; i < grid.Length; ++i)
			{
				for (int j = 0; j < grid[i].Length; ++j)
				{
					if (grid[i][j].Count == 0)
					{
						Console.Write(" ");
						continue;
					}
					List<GameObject> cell = grid[i][j];
					Console.Write(cell[cell.Count-1].ToString());
				}
				Console.WriteLine();
			}
			
			// TODO
			// GameState state = new GameState(grid, babas);
		}
	}
}
