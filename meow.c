//meow
/*meow*/

#include<stdio.h>
#include<stdlib.h>

void clearScreen()
{
    system("cls");
}

void student();

int main(){
    int option;
    printf("1.Login as student\n2.Login as faculty\n3.Login as admin\n4.Forgot password\n5.Exit\n");
    printf("Enter your option");
    scanf("%d", &option);
    if(option == 1){
        getchar();
        student();
    }
}


void student(){
    clearScreen();
    printf("Hello student");
    getchar();
}