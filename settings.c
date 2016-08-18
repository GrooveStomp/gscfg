#include <string.h> /* strncmp */
typedef struct my_struct
{
        char *markdown;
        char *highlighter;
        char *permalink;
        char *title;
        char *name;
        char *tagline;
        char *description;
        char *url;
        struct
        {
                char *name;
                char *url;
        } author;
        char *paginate;
        char *version;
        struct
        {
                char *repo;
        } github;
        char *boardgames;
        char *photos;
        struct
        {
                char *twitter;
                char *github;
                char *bitbucket;
                struct
                {
                        char *groovestomp;
                        char *aaron;
                } icon;
        } profile;
        char *exclude;
} my_struct;
void
MyStructInit(my_struct *Self)
{
        Self->markdown = "kramdown";
        Self->highlighter = "rouge";
        Self->permalink = "pretty";
        Self->title = "GrooveStomp";
        Self->name = "'GrooveStomp: Online Ruminations'";
        Self->tagline = "Online Ruminations";
        Self->description = "Thoughts on matters mostly related to software development";
        Self->url = "http://www.groovestomp.com";
        Self->author.name = "Aaron Oman";
        Self->author.url = "http://www.groovestomp.com/about";
        Self->paginate = "5";
        Self->version = "2.0.0";
        Self->github.repo = "https://github.com/poole/hyde";
        Self->boardgames = "http://boardgames.groovestomp.com";
        Self->photos = "https://groovestomp.smugmug.com";
        Self->profile.twitter = "https://twitter.com/GrooveStomp";
        Self->profile.github = "https://github.com/GrooveStomp";
        Self->profile.bitbucket = "https://bitbucket.org/GrooveStomp";
        Self->profile.icon.groovestomp = "/public/groovestomp_icon.jpg";
        Self->profile.icon.aaron = "/public/profile.jpg";
        Self->exclude = "[vendor, env]";
}
unsigned int
MyStructHasKey(my_struct *Self, char *String)
{
        if(strncmp(String, "markdown", 8) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "highlighter", 11) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "permalink", 9) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "title", 5) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "name", 4) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "tagline", 7) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "description", 11) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "url", 3) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "author.name", 11) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "author.url", 10) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "paginate", 8) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "version", 7) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "github.repo", 11) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "boardgames", 10) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "photos", 6) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "profile.twitter", 15) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "profile.github", 14) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "profile.bitbucket", 17) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "profile.icon.groovestomp", 24) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "profile.icon.aaron", 18) == 0)
        {
                return(!0);
        }
        if(strncmp(String, "exclude", 7) == 0)
        {
                return(!0);
        }
        return(0);
}
char *
MyStructGet(my_struct *Self, char *String)
{
        if(strncmp(String, "markdown", 8) == 0)
        {
                return(Self->markdown);
        }
        if(strncmp(String, "highlighter", 11) == 0)
        {
                return(Self->highlighter);
        }
        if(strncmp(String, "permalink", 9) == 0)
        {
                return(Self->permalink);
        }
        if(strncmp(String, "title", 5) == 0)
        {
                return(Self->title);
        }
        if(strncmp(String, "name", 4) == 0)
        {
                return(Self->name);
        }
        if(strncmp(String, "tagline", 7) == 0)
        {
                return(Self->tagline);
        }
        if(strncmp(String, "description", 11) == 0)
        {
                return(Self->description);
        }
        if(strncmp(String, "url", 3) == 0)
        {
                return(Self->url);
        }
        if(strncmp(String, "author.name", 11) == 0)
        {
                return(Self->author.name);
        }
        if(strncmp(String, "author.url", 10) == 0)
        {
                return(Self->author.url);
        }
        if(strncmp(String, "paginate", 8) == 0)
        {
                return(Self->paginate);
        }
        if(strncmp(String, "version", 7) == 0)
        {
                return(Self->version);
        }
        if(strncmp(String, "github.repo", 11) == 0)
        {
                return(Self->github.repo);
        }
        if(strncmp(String, "boardgames", 10) == 0)
        {
                return(Self->boardgames);
        }
        if(strncmp(String, "photos", 6) == 0)
        {
                return(Self->photos);
        }
        if(strncmp(String, "profile.twitter", 15) == 0)
        {
                return(Self->profile.twitter);
        }
        if(strncmp(String, "profile.github", 14) == 0)
        {
                return(Self->profile.github);
        }
        if(strncmp(String, "profile.bitbucket", 17) == 0)
        {
                return(Self->profile.bitbucket);
        }
        if(strncmp(String, "profile.icon.groovestomp", 24) == 0)
        {
                return(Self->profile.icon.groovestomp);
        }
        if(strncmp(String, "profile.icon.aaron", 18) == 0)
        {
                return(Self->profile.icon.aaron);
        }
        if(strncmp(String, "exclude", 7) == 0)
        {
                return(Self->exclude);
        }
        return(NULL);
}
