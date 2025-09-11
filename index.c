#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  // for seeding rand with the current time — avoids same timetable on each run
#include <ctype.h> // for case-insensitive matching and trimming inputs easily

#define MAX_SUBJECTS 5
#define MAX_DAYS 7
#define MAX_PERIODS 10
#define MAX_NAME_LENGTH 20
#define MAX_FACULTY 5
#define MAX_SECTIONS 2

// struct to hold subject info — name, how many classes, constraints on days, and faculty name
typedef struct
{
    char name[MAX_NAME_LENGTH];
    int classes_per_week;
    char constraints[MAX_DAYS][MAX_NAME_LENGTH];
    int num_constraints;
    char faculty[MAX_NAME_LENGTH];
} Subject;

// struct for each section's timetable and name
typedef struct
{
    char timetable[MAX_DAYS][MAX_PERIODS][MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
} Section;

// helper to clear the input buffer — useful after scanf to avoid leftover newlines messing up fgets
void clear_input_buffer()
{
    while (getchar() != '\n' && !feof(stdin))
        ;
}

// make a string lowercase so we can compare without worrying how the user typed it
void to_lower_case(char *str)
{
    for (int i = 0; str[i]; i++)
        str[i] = tolower((unsigned char)str[i]);
}

// check if a faculty is already teaching at this time in another section
int is_faculty_available(const char *faculty, int day, int period,
                         const Section sections[MAX_SECTIONS], int current_section)
{
    if (strcmp(faculty, "Unassigned") == 0)
        return 1;

    for (int s = 0; s < MAX_SECTIONS; s++)
    {
        if (s == current_section)
            continue;
        if (strstr(sections[s].timetable[day][period], faculty) != NULL)
            return 0;
    }
    return 1;
}

// check if the same subject is already scheduled at this time in another section
int is_subject_clash(const char *subject, int day, int period,
                     const Section sections[MAX_SECTIONS], int current_section)
{
    for (int s = 0; s < MAX_SECTIONS; s++)
    {
        if (s == current_section)
            continue;
        if (strstr(sections[s].timetable[day][period], subject) != NULL)
            return 1;
    }
    return 0;
}

// main logic to assign subjects to timetable randomly, but respecting constraints
void distribute_subjects(Subject subjects[], int num_subjects, Section sections[],
                         int working_days, int periods_per_day, const char day_names[][MAX_NAME_LENGTH])
{
    srand(time(NULL)); // seed random once — otherwise timetable looks the same every time

    for (int s = 0; s < MAX_SECTIONS; s++)
    {
        for (int i = 0; i < num_subjects; i++)
        {
            int remaining = subjects[i].classes_per_week;
            int attempts = 0, max_attempts = 10000; // avoid infinite loops if it's too constrained

            while (remaining > 0 && attempts < max_attempts)
            {
                int d = rand() % working_days;
                int p = rand() % periods_per_day;

                if (strcmp(sections[s].timetable[d][p], "Free") != 0)
                {
                    attempts++;
                    continue;
                }

                // check if faculty is free at this time
                if (!is_faculty_available(subjects[i].faculty, d, p, sections, s))
                {
                    attempts++;
                    continue;
                }

                // check if same subject is being taught elsewhere at this time
                if (is_subject_clash(subjects[i].name, d, p, sections, s))
                {
                    attempts++;
                    continue;
                }

                // check if this day is constrained for this subject
                char lowday[MAX_NAME_LENGTH];
                strcpy(lowday, day_names[d]);
                to_lower_case(lowday);
                int skip = 0;
                for (int c = 0; c < subjects[i].num_constraints; c++)
                {
                    if (strcmp(subjects[i].constraints[c], lowday) == 0)
                    {
                        skip = 1;
                        break;
                    }
                }
                if (skip)
                {
                    attempts++;
                    continue;
                }

                // finally assign it if all checks passed
                char entry[MAX_NAME_LENGTH];
                snprintf(entry, MAX_NAME_LENGTH, "%s(%c)",
                         subjects[i].name, subjects[i].faculty[0]);
                strcpy(sections[s].timetable[d][p], entry);
                remaining--;
                attempts = 0; // reset attempts after successful assignment
            }

            if (remaining > 0)
            {
                printf("\n Could not place all classes for '%s' in %s. Try relaxing constraints.\n",
                       subjects[i].name, sections[s].name);
                exit(1);
            }
        }
    }
}

int main()
{
    printf("=== Enhanced Timetable Generator ===\n\n");

    Subject subjects[MAX_SUBJECTS];
    Section sections[MAX_SECTIONS] = {
        {.name = "Section A"},
        {.name = "Section B"}}; // initializing section names directly
    int num_subjects = 0;
    char input[100];
    char faculty_list[MAX_FACULTY][MAX_NAME_LENGTH] = {0};
    int num_faculty = 0;

    // collect faculty names — up to MAX_FACULTY, stop if user leaves it blank
    printf("Enter faculty names (up to %d):\n", MAX_FACULTY);
    for (int i = 0; i < MAX_FACULTY; i++)
    {
        printf("Faculty %d name (leave empty to finish): ", i + 1);
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0)
            break;
        strncpy(faculty_list[i], input, MAX_NAME_LENGTH - 1);
        num_faculty++;
    }

    num_subjects = num_faculty; // assume each faculty teaches one subject
    for (int i = 0; i < num_subjects; i++)
    {
        printf("\nSubject for %s:\n", faculty_list[i]);
        printf("Enter name of subject: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';
        strncpy(subjects[i].name, input, MAX_NAME_LENGTH - 1);

        printf("Enter classes per week for %s: ", subjects[i].name);
        scanf("%d", &subjects[i].classes_per_week);
        clear_input_buffer(); // clear leftover newline

        strncpy(subjects[i].faculty, faculty_list[i], MAX_NAME_LENGTH - 1);
        subjects[i].num_constraints = 0; // initialize constraints count
    }

    int working_days;
    printf("\nEnter number of working days (1-7): ");
    scanf("%d", &working_days);
    clear_input_buffer();

    char day_names[MAX_DAYS][MAX_NAME_LENGTH] = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

    int periods_per_day;
    printf("Enter number of periods per day: ");
    scanf("%d", &periods_per_day);
    clear_input_buffer();

    // ask user for days when subject can't be scheduled — store as lowercase for easier matching later
    printf("\nEnter subject constraints (comma-separated days, blank if none):\n");
    for (int i = 0; i < num_subjects; i++)
    {
        printf("Days when %s is NOT available: ", subjects[i].name);
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) > 0)
        {
            char *token = strtok(input, ",");
            while (token != NULL && subjects[i].num_constraints < MAX_DAYS)
            {
                while (isspace(*token)) // trim spaces at the beginning
                    token++;
                char *end = token + strlen(token) - 1;
                while (end > token && isspace(*end)) // trim spaces at the end
                {
                    *end = '\0';
                    end--;
                }
                to_lower_case(token); // for case-insensitive comparison later
                strncpy(subjects[i].constraints[subjects[i].num_constraints],
                        token, MAX_NAME_LENGTH - 1);
                subjects[i].num_constraints++;
                token = strtok(NULL, ",");
            }
        }
    }

    // sanity check: make sure total classes don’t exceed available slots
    int total_required = 0;
    for (int i = 0; i < num_subjects; i++)
        total_required += subjects[i].classes_per_week;
    int total_available = working_days * periods_per_day * MAX_SECTIONS;
    if (total_required > total_available)
    {
        printf("\nError: Total required classes (%d) exceed available slots (%d).\n",
               total_required, total_available);
        return 1;
    }

    // check individual subject constraints — ensure scheduling is feasible
    for (int i = 0; i < num_subjects; i++)
    {
        int available_days = working_days - subjects[i].num_constraints;
        int max_slots = available_days * periods_per_day * MAX_SECTIONS;
        if (available_days < 1)
        {
            printf("\nError: Subject '%s' has no available days due to constraints.\n",
                   subjects[i].name);
            return 1;
        }
        if (subjects[i].classes_per_week > max_slots)
        {
            printf("\nError: Subject '%s' requires %d periods/week but only %d possible.\n",
                   subjects[i].name, subjects[i].classes_per_week, max_slots);
            return 1;
        }
    }

    // initialize timetable with "Free" entries
    for (int s = 0; s < MAX_SECTIONS; s++)
    {
        for (int d = 0; d < working_days; d++)
        {
            for (int p = 0; p < periods_per_day; p++)
            {
                strcpy(sections[s].timetable[d][p], "Free");
            }
        }
    }

    // do the actual scheduling work
    distribute_subjects(subjects, num_subjects, sections, working_days, periods_per_day, day_names);

    // print the resulting timetable in a readable format
    printf("\n=== Final Timetables ===\n\n");
    for (int s = 0; s < MAX_SECTIONS; s++)
    {
        printf("Section: %s\n", sections[s].name);
        for (int d = 0; d < working_days; d++)
        {
            printf("%s:\n", day_names[d]);
            for (int p = 0; p < periods_per_day; p++)
            {
                printf("  Period %d: %s\n", p + 1, sections[s].timetable[d][p]);
            }
            printf("\n");
        }
    }

    return 0;
}
