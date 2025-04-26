
//Name: Brayden Aubry, Logan Hunt, Khalil Balawi, Brent Lamplugh, Sangaa Chatterjee
//Class: CMPSC 311
//Assignment: Project (Client Side)
//Date: IDK


#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <gtk/gtk.h>

// server connection details
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_MSG_LEN 1024

// application details
#define APP_NAME "Chat App"

int sockfd; // socket file descriptor
char username[50]; //username variable to store username entry from GUI


// thread to receive messages from server
void* receive_messages(void* arg) {
    char buffer[MAX_MSG_LEN];

    while (1) {
        int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("server disconnected.\n");
            close(sockfd);
            exit(1);
        }

        buffer[bytes] = '\0';
        printf("\n%s", buffer);  // message already includes username
        printf("you: ");
        fflush(stdout);
    }

    return NULL;
}


//function to retrieve the username entry from the text box when the login_button is clicked
static void username_retrieve(GtkButton *button, gpointer data) {
    //retrieve text from entry to store as client username
    GtkEntry *entry = GTK_ENTRY(data);
    const char *username = gtk_editable_get_text(GTK_EDITABLE(entry));
    //test
    char *user_text = g_strdup(username);
    g_print("Username: %s\n", user_text);
    g_free(user_text);
}

static void on_login_button_clicked(GtkButton *button, gpointer data)
{
    GtkStack *stack = GTK_STACK(data);
    //TODO verify name and stuff? (check its not empty)
    gtk_stack_set_visible_child_name(stack, "home");
}

static void on_join_room_button_clicked(GtkButton *button, gpointer data)
{
    GtkStack *stack = GTK_STACK(data);
    //TODO verify name and stuff? (check its not empty)
    gtk_stack_set_visible_child_name(stack, "chat room");
}

// sets up the app when its activated (started)
static void on_activate(GtkApplication *app, gpointer user_data)
{
    // set up window
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 800);
    gtk_window_set_title(GTK_WINDOW(window), APP_NAME);
    gtk_window_present(GTK_WINDOW(window));

    // create page stack
    GtkWidget *stack = gtk_stack_new();
    gtk_window_set_child(GTK_WINDOW(window), stack); // the stack should always be the window's only child, don't add anything else

    // build login page
    GtkWidget *login_box;
    GtkWidget *login_button;
    GtkWidget *entry; //username entry

    login_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // button allignment
    gtk_widget_set_halign(login_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(login_box, GTK_ALIGN_CENTER);
    entry = gtk_entry_new();
    login_button = gtk_button_new_with_label("Login");

    //login_button functions
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button_clicked), stack); //calls on-login_button_fixed() when the button is clicked
    g_signal_connect(login_button, "clicked", G_CALLBACK(username_retrieve), entry); //calls username_retrieve() when the button is clicked

    gtk_box_append(GTK_BOX(login_box), entry);
    gtk_box_append(GTK_BOX(login_box), login_button);

    // build home page
    GtkWidget *home_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_halign(home_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(home_box, GTK_ALIGN_CENTER);
    GtkWidget *join_button = gtk_button_new_with_label("Join Room"); //
    GtkWidget *host_button = gtk_button_new_with_label("Host Room"); //make main() from server a fuction. Call the function here to start a server

    //TODO actually make the buttons do something
    g_signal_connect(join_button, "clicked", G_CALLBACK(on_join_room_button_clicked), stack); //calls on_join_room_button_clicked when the button is clicked

    gtk_box_append(GTK_BOX(home_box), join_button);
    gtk_box_append(GTK_BOX(home_box), host_button);


    // build chat room page
    GtkWidget *chat_room_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_halign(chat_room_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(chat_room_box, GTK_ALIGN_CENTER);




    // add pages to stack
    gtk_stack_add_named(GTK_STACK(stack), login_box, "login");
    gtk_stack_add_named(GTK_STACK(stack), home_box, "home");
    gtk_stack_add_named(GTK_STACK(stack), chat_room_box, "chat room");



    // show login page
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "login");
    gtk_window_present(GTK_WINDOW(window));
}

int main() {
    struct sockaddr_in server_addr;
    char message[MAX_MSG_LEN];
    //char username[50]; //should not need declaration here now
    pthread_t recv_thread;

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    // configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);





    // set up app window
    GtkApplication *app;
    //g_signal_conenct_(app, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL); // app is set up in the on_activate function
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);





    // connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        exit(1);
    }

    printf("connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // prompt for username and send it to server
    //printf("enter your username: ");
    fgets(username, sizeof(username), stdin);
    //username[strcspn(username, "\n")] = '\0';  // remove newline
    send(sockfd, username, strlen(username), 0);

    // start thread to receive messages
    pthread_create(&recv_thread, NULL, receive_messages, NULL);

    // loop to send messages
    while (1) {
        printf("you: ");
        if (fgets(message, MAX_MSG_LEN, stdin) != NULL) {
            send(sockfd, message, strlen(message), 0);
        }
    }

    return 0;
}
