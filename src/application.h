#ifndef APPLICATION_H
#define APPLICATION_H
#include "stdio.h"
#include "stdlib.h"


typedef struct {
	int is_running;
	int show_menu;

} Application;

Application* init_application() {
	Application *app = (Application *)malloc(sizeof(Application));
	if (app == NULL) {
		printf("Failed to allocate memory for Application\n");
		return NULL;
	}
	app->is_running = 1;
	app->show_menu = 0;

	return app;
}
void uinit_application(Application *app) {
	if (app != NULL) {
		free(app);
		app = NULL;
	}
}

void close_application(Application *app) {
	app->is_running = 0; // Set the running flag to false
}

void toggle_menu(Application *app) {
	if (app == NULL) {
		printf("Application is not initialized\n");
		return;
	}
	app->show_menu = !app->show_menu; // Toggle the menu visibility
}
#endif // APPLICATION_H
