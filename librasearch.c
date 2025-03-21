#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BOOKS 100
#define DB_FILE "books_db.txt"
#define REQUEST_FILE "requests.txt"
#define ALLOCATIONS_FILE "allocations.txt"

// Structure to store book information
typedef struct {
    int id;
    char title[100];
    char author[100];
    int allocated;
} Book;

typedef struct {
    int user_id;
    int book_id;
    int fulfilled; // 0 for pending, 1 for fulfilled, -1 for deallocated
} Request;

Book library[MAX_BOOKS];
Request requests[MAX_BOOKS];
int book_count = 0;
int request_count = 0;

// Function to calculate penalty based on extra days
int calculate_penalty(int days_issued) {
    int penalty_rate = 2;  // Penalty rate per extra day
    int grace_period = 3;  // Grace period of 3 days
    int extra_days = days_issued > grace_period ? (days_issued - grace_period) : 0;
    return extra_days * penalty_rate;
}

// Load books from DB_FILE
void load_books() {
    FILE *file = fopen(DB_FILE, "r");
    if (!file) {
        printf("Error opening database file\n");
        return;
    }
    while (fscanf(file, "%d,%99[^,],%99[^\n]\n", &library[book_count].id, library[book_count].title, library[book_count].author) == 3) {
        library[book_count].allocated = 0; // Initially, mark all as unallocated
        book_count++;
    }
    fclose(file);
}

// Load requests from REQUEST_FILE
void load_requests() {
    FILE *file = fopen(REQUEST_FILE, "r");
    if (!file) {
        printf("Error opening requests file\n");
        return;
    }
    while (fscanf(file, "%d,%d\n", &requests[request_count].user_id, &requests[request_count].book_id) == 2) {
        requests[request_count].fulfilled = 0; // Initially, mark all as pending
        request_count++;
    }
    fclose(file);
}

// Check if book ID is unique
int is_unique_id(int id) {
    for (int i = 0; i < book_count; i++) {
        if (library[i].id == id) {
            return 0; // ID is not unique
        }
    }
    return 1; // ID is unique
}

// Add a new book to the database
void add_book() {
    int id;
    char title[100], author[100];

    do {
        printf("Enter a unique Book ID: ");
        scanf("%d", &id);

        if (!is_unique_id(id)) {
            printf("Error: Book ID %d already exists. Please enter a unique ID.\n", id);
        }
    } while (!is_unique_id(id)); // Repeat until a unique ID is entered

    getchar();  // Consume newline left by scanf
    printf("Enter Book Title: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = '\0';  // Remove newline character
    printf("Enter Author Name: ");
    fgets(author, sizeof(author), stdin);
    author[strcspn(author, "\n")] = '\0';  // Remove newline character

    FILE *file = fopen(DB_FILE, "a");
    if (!file) {
        printf("Error opening database file\n");
        return;
    }
    fprintf(file, "%d,%s,%s\n", id, title, author);
    fclose(file);

    library[book_count].id = id;
    strcpy(library[book_count].title, title);
    strcpy(library[book_count].author, author);
    library[book_count].allocated = 0;
    book_count++;

    printf("Book added successfully.\n");
}

// Display available books
void display_available_books() {
    printf("\nAvailable Books:\n");
    printf("ID\tTitle\t\tAuthor\n");
    printf("------------------------------------\n");
    for (int i = 0; i < book_count; i++) {
        if (!library[i].allocated) {
            printf("%d\t%s\t\t%s\n", library[i].id, library[i].title, library[i].author);
        }
    }
}

// Add a new book request to REQUEST_FILE
void add_book_request() {
    int user_id, book_id;
    printf("Enter User ID: ");
    scanf("%d", &user_id);
    printf("Enter Book ID: ");
    scanf("%d", &book_id);

    FILE *requests_file = fopen(REQUEST_FILE, "a");
    if (!requests_file) {
        printf("Error opening request file\n");
        return;
    }
    fprintf(requests_file, "%d,%d\n", user_id, book_id);
    fclose(requests_file);

    requests[request_count].user_id = user_id;
    requests[request_count].book_id = book_id;
    requests[request_count].fulfilled = 0;
    request_count++;

    printf("Request added successfully.\n");
}

// Allocate books based on pending requests
void allocate_books() {
    FILE *allocations = fopen(ALLOCATIONS_FILE, "a");
    if (!allocations) {
        printf("Error opening allocations file\n");
        return;
    }

    for (int i = 0; i < request_count; i++) {
        if (requests[i].fulfilled) {
            continue; // Skip fulfilled requests
        }

        int found = 0;
        for (int j = 0; j < book_count; j++) {
            if (library[j].id == requests[i].book_id && !library[j].allocated) {
                fprintf(allocations, "User %d allocated Book ID %d (%s)\n", requests[i].user_id, library[j].id, library[j].title);
                library[j].allocated = 1;
                requests[i].fulfilled = 1;
                found = 1;
                break;
            }
        }

        if (!found) {
            fprintf(allocations, "User %d request for Book ID %d could not be fulfilled (not available)\n", requests[i].user_id, requests[i].book_id);
        }
    }

    fclose(allocations);
    printf("Book allocation complete. Check allocations.txt for details.\n");
}

// Deallocate a book and calculate penalty
void deallocate_books() {
    int user_id, book_id, days_issued;
    printf("Enter User ID: ");
    scanf("%d", &user_id);
    printf("Enter Book ID to deallocate: ");
    scanf("%d", &book_id);
    printf("Enter the number of days the book was issued: ");
    scanf("%d", &days_issued);

    int found = 0;
    for (int i = 0; i < book_count; i++) {
        if (library[i].id == book_id && library[i].allocated) {
            // Verify if the user is the one who actually allocated this book
            int valid_user = 0;
            for (int j = 0; j < request_count; j++) {
                if (requests[j].user_id == user_id && requests[j].book_id == book_id && requests[j].fulfilled == 1) {
                    valid_user = 1;
                    requests[j].fulfilled = -1; // Mark as deallocated
                    break;
                }
            }
            if (!valid_user) {
                printf("Deallocation failed: Book ID %d was not allocated to User %d.\n", book_id, user_id);
                return;
            }

            int penalty = calculate_penalty(days_issued);
            library[i].allocated = 0; // Deallocate the book
            found = 1;
            printf("Book ID %d (%s) has been deallocated.\n", book_id, library[i].title);
            printf("Penalty for keeping the book beyond the grace period: %d\n", penalty);
            break;
        }
    }

    if (!found) {
        printf("Book not found or already deallocated.\n");
    } else {
        printf("\nRetrying pending allocation requests...\n");
        allocate_books();
    }
}

// Clear request and allocation files on exit
void clear_files() {
    FILE *requests_file = fopen(REQUEST_FILE, "w");
    FILE *allocations_file = fopen(ALLOCATIONS_FILE, "w");

    if (requests_file) fclose(requests_file);
    if (allocations_file) fclose(allocations_file);

    printf("All requests and allocations cleared.\n");
}

// Main menu function
void menu() {
    int choice;
    load_books();     // Load books from the database file
    load_requests();  // Load requests from the request file

    do {
        printf("\n--- LibraSearch ---\n");
        printf("1. Add a Book\n");
        printf("2. Display Available Books\n");
        printf("3. Add Book Request\n");
        printf("4. Allocate Books\n");
        printf("5. Deallocate (Return) a Book\n");
        printf("6. Quit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                add_book();
                break;
            case 2:
                display_available_books();
                break;
            case 3:
                add_book_request();
                break;
            case 4:
                allocate_books();
                break;
            case 5:
                deallocate_books();
                break;
            case 6:
                clear_files(); // Clear files before exiting
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice\n");
        }
    } while (choice != 6);
}

int main() {
    menu();
    return 0;
}
