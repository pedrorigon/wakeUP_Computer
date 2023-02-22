<h1> WakeUP Computer Project </h1>

> Status: Developing

### The practical work proposal is to implement a "sleep management" service for workstations that belong to the same physical network segment of a large organization. The goal of the service is to promote energy savings by encouraging employees to put their workstations to sleep after the end of the workday in the organization. In this case, the service must ensure that the workstations can be awakened if the employee wishes to remotely access a service on their workstation (for example, a file-sharing service to access personal files). This project was undertaken as a criterion for evaluation in the subject of Operating Systems 2 at UFRGS. The objective of the work is to implement a "sleep management" service for workstations that belong to the same physical network segment of a large organization. 

> The work proposal is inspired by GreenUp, a solution for monitoring energy consumption and sleep management of workstations developed by Microsoft Research:
```
https://www.microsoft.com/en-us/research/project/greenup/
```

# Tutorial on how to run the program

To compile and execute the program, we have created a .sh script to automate the compilation and execution tasks. Here's a tutorial on how to run the program via the script located in the attached ZIP file and named run_program.sh

1. Open the terminal in Ubuntu (by pressing "Ctrl + Alt + T" on the keyboard).
2. Navigate to the folder where the "run_program.sh" script is saved.
3. Check if the "run_program.sh" script has permission to be executed. If you haven't granted permission yet, use the following command to give execution permission:
```
chmod +x run_program.sh
```
4. Now, you can run the script with or without the "manager" parameter. 
- To run the script as a participant, simply type the following command:
  ```
  ./run_program.sh
  ```
- If you want to run the script as a "manager", simply type the following command:
  ```
  ./run_program.sh manager
  ```
