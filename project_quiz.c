#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct Question {
    int id;
    char topic[30];
    char question[200];
    char options[4][50];
    char correctAnswer;
};

struct Player {
    char name[50];
    int score;
};

void getFileNames(const char *subject, char *quizFile, char *scoreFile) {
    if (quizFile) sprintf(quizFile, "%s_quiz.dat", subject);
    if (scoreFile) sprintf(scoreFile, "%s_scores.dat", subject);
}

int isSubjectListed(const char *subject) {
    FILE *file = fopen("subjects.txt", "r");
    if (!file) return 0;
    char line[30];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, subject) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

void addSubjectIfNew(const char *subject) {
    if (!isSubjectListed(subject)) {
        FILE *file = fopen("subjects.txt", "a");
        if (file) {
            fprintf(file, "%s\n", subject);
            fclose(file);
        }
    }
}

void listSubjects() {
    printf("\nAvailable Subjects:\n");
    FILE *file = fopen("subjects.txt", "r");
    if (!file) {
        printf("  (No subjects found. Please add one.)\n");
        return;
    }
    char subject[30];
    int count = 0;
    while (fgets(subject, sizeof(subject), file)) {
        subject[strcspn(subject, "\n")] = '\0';
        printf("  %d. %s\n", ++count, subject);
    }
    fclose(file);
}

int questionIDExists(FILE *file, int id) {
    struct Question q;
    while (fread(&q, sizeof(q), 1, file)) {
        if (q.id == id) return 1;
    }
    return 0;
}

void addQuestion() {
    char subject[30], quizFile[50];
    printf("Enter subject name (e.g., c, math): ");
    scanf("%s", subject);
    getchar();
    addSubjectIfNew(subject);
    getFileNames(subject, quizFile, NULL);
    FILE *file = fopen(quizFile, "ab+");
    if (!file) {
        printf("Error opening file!\n");
        return;
    }

    struct Question q;
    printf("Enter unique question ID: ");
    scanf("%d", &q.id);
    getchar();
    rewind(file);
    if (questionIDExists(file, q.id)) {
        printf("Question ID already exists!\n");
        fclose(file);
        return;
    }

    printf("Enter topic: ");
    fgets(q.topic, 30, stdin);
    q.topic[strcspn(q.topic, "\n")] = '\0';
    printf("Enter question: ");
    fgets(q.question, 200, stdin);
    q.question[strcspn(q.question, "\n")] = '\0';

    for (int i = 0; i < 4; i++) {
        printf("Enter option %c: ", 'A' + i);
        fgets(q.options[i], 50, stdin);
        q.options[i][strcspn(q.options[i], "\n")] = '\0';
    }

    do {
        printf("Enter correct answer (A/B/C/D): ");
        scanf(" %c", &q.correctAnswer);
        q.correctAnswer = toupper(q.correctAnswer);
    } while (q.correctAnswer < 'A' || q.correctAnswer > 'D');

    fwrite(&q, sizeof(q), 1, file);
    fclose(file);
    printf("Question added to '%s'.\n", subject);
}

void takeQuiz() {
    char subject[30], quizFile[50], scoreFile[50];
    listSubjects();
    printf("\nEnter subject to take quiz: ");
    scanf("%s", subject);
    getFileNames(subject, quizFile, scoreFile);

    FILE *file = fopen(quizFile, "rb");
    if (!file) {
        printf("No quiz found for subject '%s'.\n", subject);
        return;
    }

    struct Question q;
    struct Player p;
    printf("Enter your name: ");
    scanf("%s", p.name);
    p.score = 0;

    int total = 0;
    fseek(file, 0, SEEK_END);
    total = ftell(file) / sizeof(struct Question);
    rewind(file);

    int current = 1;
    while (fread(&q, sizeof(q), 1, file)) {
        printf("\n[%d/%d] Topic: %s\n", current++, total, q.topic);
        printf("%s\n", q.question);
        for (int i = 0; i < 4; i++)
            printf("%c. %s\n", 'A' + i, q.options[i]);

        char answer;
        do {
            printf("Your answer (A/B/C/D): ");
            scanf(" %c", &answer);
            answer = toupper(answer);
        } while (answer < 'A' || answer > 'D');

        if (answer == q.correctAnswer) {
            printf("Correct!\n");
            p.score += 10;
        } else {
            printf("Wrong! Correct: %c\n", q.correctAnswer);
        }
    }
    fclose(file);

    file = fopen(scoreFile, "ab");
    fwrite(&p, sizeof(p), 1, file);
    fclose(file);
    printf("\nQuiz Over! %s, your score: %d/%d\n", p.name, p.score, total * 10);
}

void viewLeaderboard() {
    char subject[30], scoreFile[50];
    listSubjects();
    printf("\nEnter subject to view leaderboard: ");
    scanf("%s", subject);
    getFileNames(subject, NULL, scoreFile);

    FILE *file = fopen(scoreFile, "rb");
    if (!file) {
        printf("No leaderboard found for '%s'.\n", subject);
        return;
    }

    struct Player players[100];
    int count = 0;
    while (fread(&players[count], sizeof(struct Player), 1, file)) {
        count++;
    }
    fclose(file);

    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (players[j].score < players[j + 1].score) {
                struct Player temp = players[j];
                players[j] = players[j + 1];
                players[j + 1] = temp;
            }
        }
    }

    printf("\nLeaderboard for '%s':\n", subject);
    for (int i = 0; i < count; i++) {
        printf("  %d. %s - %d\n", i + 1, players[i].name, players[i].score);
    }
}

void deleteQuestion() {
    char subject[30], quizFile[50];
    listSubjects();
    printf("\nEnter subject to delete question from: ");
    scanf("%s", subject);
    getFileNames(subject, quizFile, NULL);

    FILE *file = fopen(quizFile, "rb");
    FILE *temp = fopen("temp.dat", "wb");
    if (!file || !temp) {
        printf("Error opening files!\n");
        return;
    }

    int id, found = 0;
    printf("Enter question ID to delete: ");
    scanf("%d", &id);

    struct Question q;
    while (fread(&q, sizeof(q), 1, file)) {
        if (q.id == id) found = 1;
        else fwrite(&q, sizeof(q), 1, temp);
    }

    fclose(file);
    fclose(temp);
    remove(quizFile);
    rename("temp.dat", quizFile);
    if (found) printf("Question deleted!\n");
    else printf("Question ID not found.\n");
}

void resetLeaderboard() {
    char subject[30], scoreFile[50];
    listSubjects();
    printf("\nEnter subject to reset leaderboard: ");
    scanf("%s", subject);
    getFileNames(subject, NULL, scoreFile);

    FILE *file = fopen(scoreFile, "wb");
    if (file) {
        fclose(file);
        printf("Leaderboard for '%s' reset.\n", subject);
    } else {
        printf("Error resetting leaderboard.\n");
    }
}

void importQuestionsFromText() {
    char subject[30], quizFile[50], fileName[50];
    printf("Enter subject name: ");
    scanf("%s", subject);
    getchar();
    getFileNames(subject, quizFile, NULL);
    addSubjectIfNew(subject);

    printf("Enter the text file name (e.g., questions.txt): ");
    scanf("%s", fileName);

    FILE *txt = fopen(fileName, "r");
    FILE *dat = fopen(quizFile, "ab");
    if (!txt || !dat) {
        printf("Error opening files!\n");
        return;
    }

    struct Question q;
    char line[256];
    int fieldCount = 0;

    while (fgets(line, sizeof(line), txt)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "ID:", 3) == 0) {
            sscanf(line, "ID: %d", &q.id);
            fieldCount = 1;
        } else if (strncmp(line, "Topic:", 6) == 0) {
            sscanf(line + 6, " %[^\n]", q.topic);
            fieldCount++;
        } else if (strncmp(line, "Question:", 9) == 0) {
            sscanf(line + 9, " %[^\n]", q.question);
            fieldCount++;
        } else if (line[1] == ')' && fieldCount >= 3 && fieldCount < 7) {
            strcpy(q.options[fieldCount - 3], line + 3);
            fieldCount++;
        } else if (strncmp(line, "Answer:", 7) == 0) {
            sscanf(line, "Answer: %c", &q.correctAnswer);
            q.correctAnswer = toupper(q.correctAnswer);
            fieldCount++;
            if (fieldCount == 8) {
                fwrite(&q, sizeof(q), 1, dat);
                fieldCount = 0;
            }
        }
    }

    fclose(txt);
    fclose(dat);
    printf("Questions imported successfully to '%s'.\n", quizFile);
}

void deleteSubject() {
    char subject[30], quizFile[50], scoreFile[50];
    listSubjects();
    printf("\nEnter subject to delete: ");
    scanf("%s", subject);
    getFileNames(subject, quizFile, scoreFile);

    FILE *src = fopen("subjects.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    int found = 0;

    if (!src || !temp) {
        printf("Error opening subject list!\n");
        return;
    }

    char line[30];
    while (fgets(line, sizeof(line), src)) {
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, subject) != 0) {
            fprintf(temp, "%s\n", line);
        } else {
            found = 1;
        }
    }

    fclose(src);
    fclose(temp);
    remove("subjects.txt");
    rename("temp.txt", "subjects.txt");

    if (found) {
        remove(quizFile);
        remove(scoreFile);
        printf("Subject '%s' and its associated files deleted.\n", subject);
    } else {
        printf("Subject not found.\n");
    }
}

int main() {
    int choice;
    do {
        printf("\n========= Quiz System Menu =========\n");
        printf("1. Add Question Manually\n");
        printf("2. Take Quiz\n");
        printf("3. View Leaderboard\n");
        printf("4. Delete Question\n");
        printf("5. Reset Leaderboard\n");
        printf("6. Import Questions from Text File\n");
        printf("7. Exit\n");
        printf("8. Delete Subject\n");
        printf("====================================\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: addQuestion(); break;
            case 2: takeQuiz(); break;
            case 3: viewLeaderboard(); break;
            case 4: deleteQuestion(); break;
            case 5: resetLeaderboard(); break;
            case 6: importQuestionsFromText(); break;
            case 7: printf("Goodbye!\n"); break;
            case 8: deleteSubject(); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 7);
    return 0;
}

