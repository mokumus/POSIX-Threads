struct Student
{
  char name[64];
  int quality;
  int speed;
  int cost;

  //status variables
  int homeworks_done;
  int money_made;
  int is_busy;
  char assigned_hw;
};

int n_students;
struct Student *students;


int select_student(char hw_type)
{
  int ret = -1;
  int val = -1;
  int cost = 999999;
  for (int i = 0; i < n_students; i++)
  {
    int temp_cost = get_value(i, 'C');
    //printf("cost: %d\n", temp_cost);
    if (students[i].is_busy)
      continue;
    else if (hw_type == 'C')
    {
      if (cost > temp_cost)
      {
        cost = temp_cost;
        ret = i;
      }
    }
    else
    {
      int temp = get_value(i, hw_type);
      //printf("temp: %d\n", temp);
      if (val <= temp && cost > temp_cost)
      {
        val = temp;
        ret = i;
      }
      else if (val < temp)
      {
        val = temp;
        ret = i;
      }
    }
  }

  //printf("selected %d for job %c\n", ret, hw_type);
  return ret;
}

int get_value(int i, char key)
{
  if (i >= n_students)
    errExit("Index out of bounds");

  switch (key)
  {
  case 'Q':
    return students[i].quality;
  case 'S':
    return students[i].speed;
  case 'C':
    return students[i].cost;
  default:
    errExit("Invalid key");
    ;
  }
}