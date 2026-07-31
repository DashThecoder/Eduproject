// meow
/*meow*/

#include <stdio.h>
#include <stdlib.h>

void clearScreen()
{
    system("cls");
}

void student();

int start_menu(){
    int option;
    printf("1.Login as student\n2.Login as faculty\n3.Login as admin\n4.Forgot password\n5.Exit\n");
    printf("Enter your option:");
    scanf("%d", &option);
    if(option == 1){
        student();
    }
}

int main()
{
    start_menu();
    return 0;
}

void student()
{
    clearScreen();
    printf("1.Offerings\n2.Courses\n3.Report card\n4.Log out\nEnter your option:");
    int option;
    scanf("%d", &option);
    if(option == 1){
        clearScreen();
        int semn;
        scanf("Enter semester number: %d", &semn);
        //show sem offerings from semn
        printf("1.Search\n2.Enroll in course\n3.Withdraw course\n4. Go back\nEnter an option:");
        int semno;
        scanf("%d", &semno);
        if(semno == 4){
            clearScreen();
            printf("1.Offerings\n2.Courses\n3.Report card\n4.Log out\nEnter your option:");
            int option;
            scanf("%d", &option);
        }
    }
}