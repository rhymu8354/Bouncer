/**
 * @file UsersStore.cpp
 *
 * This module contains the implementation of the Bouncer::UsersStore
 * class.
 */

#include <Bouncer/User.hpp>
#include <Bouncer/UsersStore.hpp>
#include <Bouncer/UserStore.hpp>
#include <inttypes.h>
#include <memory>
#include <sqlite3.h>
#include <sstream>
#include <stdint.h>
#include <string>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/File.hpp>
#include <unordered_map>
#include <vector>

namespace {

    using namespace Bouncer;

    using DatabaseConnection = std::unique_ptr< sqlite3, std::function< void(sqlite3*) > >;
    using PreparedStatement = std::unique_ptr< sqlite3_stmt, std::function< void(sqlite3_stmt*) > >;

    struct UserColumn {
        std::string name;
        std::string type;
    };
    const std::vector< UserColumn > userColumns{
        {"bot", "INTEGER"},
        {"createdAt", "REAL"},
        {"firstMessageTime", "REAL"},
        {"firstSeenTime", "REAL"},
        {"id", "INTEGER PRIMARY KEY"},
        {"isBanned", "BOOLEAN"},
        {"isWhitelisted", "BOOLEAN"},
        {"lastMessageTime", "REAL"},
        {"login", "TEXT"},
        {"name", "TEXT"},
        {"note", "TEXT"},
        {"numMessages", "INTEGER"},
        {"role", "INTEGER"},
        {"timeout", "REAL"},
        {"totalViewTime", "REAL"},
        {"watching", "BOOLEAN"},
    };

}

namespace Bouncer {

    /**
     * This defines the private properties of the Bouncer::UsersStore class.
     */
    struct UsersStore::Impl {
        // Types

        struct StepStatementResults {
            bool done = false;
            bool error = false;
        };

        // Properties

        PreparedStatement getUsers;
        PreparedStatement getChat;
        PreparedStatement createUser;
        PreparedStatement addChat;
        PreparedStatement dropOldChat;
        std::unordered_map< std::string, PreparedStatement > updateUser;
        DatabaseConnection db;
        std::string dbFilePath;

        /**
         * This is a helper object used to generate and publish
         * diagnostic messages.
         */
        SystemAbstractions::DiagnosticsSender diagnosticsSender;

        std::unordered_map< std::string, intmax_t > userIdsByLogin;
        std::unordered_map< intmax_t, std::shared_ptr< UserStore > > usersById;

        // Lifecycle

        ~Impl() noexcept = default;
        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        // Methods

        Impl()
            : diagnosticsSender("UsersStore")
        {
        }

        void BindStatementParameter(
            const PreparedStatement& stmt,
            int index,
            bool value
        ) {
            (void)sqlite3_bind_int(
                stmt.get(),
                index,
                value ? 1 : 0
            );
        }

        void BindStatementParameter(
            const PreparedStatement& stmt,
            int index,
            User::Bot value
        ) {
            int valueAsInt = 0;
            switch (value) {
                case User::Bot::Unknown: {
                    valueAsInt = 0;
                } break;
                case User::Bot::Yes: {
                    valueAsInt = 1;
                } break;
                case User::Bot::No: {
                    valueAsInt = 2;
                } break;
                default: break;
            }
            (void)sqlite3_bind_int(
                stmt.get(),
                index,
                valueAsInt
            );
        }

        void BindStatementParameter(
            const PreparedStatement& stmt,
            int index,
            User::Role value
        ) {
            int valueAsInt = 0;
            switch (value) {
                case User::Role::Unknown: {
                    valueAsInt = 0;
                } break;
                case User::Role::Pleb: {
                    valueAsInt = 1;
                } break;
                case User::Role::VIP: {
                    valueAsInt = 2;
                } break;
                case User::Role::Moderator: {
                    valueAsInt = 3;
                } break;
                case User::Role::Broadcaster: {
                    valueAsInt = 4;
                } break;
                case User::Role::Admin: {
                    valueAsInt = 5;
                } break;
                case User::Role::Staff: {
                    valueAsInt = 6;
                } break;
                default: break;
            }
            (void)sqlite3_bind_int(
                stmt.get(),
                index,
                valueAsInt
            );
        }

        void BindStatementParameter(
            const PreparedStatement& stmt,
            int index,
            intmax_t value
        ) {
            (void)sqlite3_bind_int64(
                stmt.get(),
                index,
                value
            );
        }

        void BindStatementParameter(
            const PreparedStatement& stmt,
            int index,
            double value
        ) {
            (void)sqlite3_bind_double(
                stmt.get(),
                index,
                value
            );
        }

        void BindStatementParameter(
            const PreparedStatement& stmt,
            int index,
            const std::string& value
        ) {
            (void)sqlite3_bind_text(
                stmt.get(),
                index,
                value.data(),
                (int)value.length(),
                SQLITE_TRANSIENT
            );
        }

        PreparedStatement BuildStatement(
            const std::string& statement
        ) {
            sqlite3_stmt* statementRaw;
            if (
                sqlite3_prepare_v2(
                    db.get(),
                    statement.c_str(),
                    (int)(statement.length() + 1), // sqlite wants count to include the null
                    &statementRaw,
                    NULL
                )
                != SQLITE_OK
            ) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "Error preparing SQL statement \"%s\": %s",
                    statement.c_str(),
                    sqlite3_errmsg(db.get())
                );
                return nullptr;
            }
            return PreparedStatement(
                statementRaw,
                [](sqlite3_stmt* statementRaw){
                    (void)sqlite3_finalize(statementRaw);
                }
            );
        }

        void CloseDatabase() {
            getUsers = nullptr;
            getChat = nullptr;
            createUser = nullptr;
            addChat = nullptr;
            dropOldChat = nullptr;
            updateUser.clear();
            db = nullptr;
        }

        bool ExecuteStatement(
            const std::string& statement
        ) {
            char* errmsg = NULL;
            const auto result = sqlite3_exec(
                db.get(),
                statement.c_str(),
                NULL, NULL, &errmsg
            );
            if (result != SQLITE_OK) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "Error executing SQL statement \"%s\": %s",
                    statement.c_str(),
                    errmsg
                );
                sqlite3_free(errmsg);
                return false;
            }
            return true;
        }

        bool FetchColumnBoolean(
            const PreparedStatement& stmt,
            int index
        ) {
            return (sqlite3_column_int(stmt.get(), index) != 0);
        }

        User::Bot FetchColumnBot(
            const PreparedStatement& stmt,
            int index
        ) {
            switch (sqlite3_column_int(stmt.get(), index)) {
                case 1: return User::Bot::Yes;
                case 2: return User::Bot::No;
                default: return User::Bot::Unknown;
            }
        }

        double FetchColumnDouble(
            const PreparedStatement& stmt,
            int index
        ) {
            return sqlite3_column_double(stmt.get(), index);
        }

        intmax_t FetchColumnInt(
            const PreparedStatement& stmt,
            int index
        ) {
            return sqlite3_column_int64(stmt.get(), index);
        }

        User::Role FetchColumnRole(
            const PreparedStatement& stmt,
            int index
        ) {
            switch (sqlite3_column_int(stmt.get(), index)) {
                case 1: return User::Role::Pleb;
                case 2: return User::Role::VIP;
                case 3: return User::Role::Moderator;
                case 4: return User::Role::Broadcaster;
                case 5: return User::Role::Admin;
                case 6: return User::Role::Staff;
                default: return User::Role::Unknown;
            }
        }

        std::string FetchColumnString(
            const PreparedStatement& stmt,
            int index
        ) {
            return (const char*)sqlite3_column_text(stmt.get(), index);
        }

        void GetUsers(const std::shared_ptr< UserStoreContainer >& container) {
            ResetStatement(getUsers);
            std::unordered_map< intmax_t, User > users;
            while (StepStatement(getUsers)) {
                User user;
                user.bot = FetchColumnBot(getUsers, 0);
                user.createdAt = FetchColumnDouble(getUsers, 1);
                user.firstMessageTime = FetchColumnDouble(getUsers, 2);
                user.firstSeenTime = FetchColumnDouble(getUsers, 3);
                user.id = FetchColumnInt(getUsers, 4);
                user.isBanned = FetchColumnBoolean(getUsers, 5);
                user.isWhitelisted = FetchColumnBoolean(getUsers, 6);
                user.lastMessageTime = FetchColumnDouble(getUsers, 7);
                user.login = FetchColumnString(getUsers, 8);
                user.name = FetchColumnString(getUsers, 9);
                user.note = FetchColumnString(getUsers, 10);
                user.numMessages = FetchColumnInt(getUsers, 11);
                user.role = FetchColumnRole(getUsers, 12);
                user.timeout = FetchColumnDouble(getUsers, 13);
                user.totalViewTime = FetchColumnDouble(getUsers, 14);
                user.watching = FetchColumnBoolean(getUsers, 15);
                users[user.id] = user;
            }
            ResetStatement(getChat);
            while (StepStatement(getChat)) {
                const auto userid = FetchColumnInt(getChat, 0);
                const auto message = FetchColumnString(getChat, 1);
                users[userid].lastChat.push_back(message);
            }
            for (const auto& usersEntry: users) {
                const auto& user = usersEntry.second;
                auto userStore = std::make_shared< UserStore >(user, container);
                userIdsByLogin[user.login] = user.id;
                usersById[user.id] = std::move(userStore);
            }
        };

        bool OpenDatabase() {
            sqlite3* dbRaw;
            if (sqlite3_open(dbFilePath.c_str(), &dbRaw) != SQLITE_OK) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "Unable to open database \"%s\": %s",
                    dbFilePath.c_str(),
                    sqlite3_errmsg(dbRaw)
                );
                (void)sqlite3_close(dbRaw);
                return false;
            }
            db = DatabaseConnection(
                dbRaw,
                [](sqlite3* dbRaw){
                    (void)sqlite3_close(dbRaw);
                }
            );
            std::ostringstream userColumnNames, userColumnValuePlaceholders;
            bool firstColumn = true;
            for (const auto& userColumn: userColumns) {
                if (!firstColumn) {
                    userColumnNames << ',';
                    userColumnValuePlaceholders << ',';
                }
                userColumnNames << userColumn.name;
                userColumnValuePlaceholders << '?';
                firstColumn = false;
                auto updateUserThisColumn = BuildStatement(
                    StringExtensions::sprintf(
                        "UPDATE users SET %s = ? WHERE id = ?",
                        userColumn.name.c_str()
                    )
                );
                if (updateUserThisColumn == nullptr) {
                    CloseDatabase();
                    return false;
                } else {
                    updateUser[userColumn.name] = std::move(updateUserThisColumn);
                }
            }
            getUsers = BuildStatement(
                StringExtensions::sprintf(
                    "SELECT %s FROM users",
                    userColumnNames.str().c_str()
                )
            );
            getChat = BuildStatement(
                "SELECT userid, message FROM chat"
            );
            createUser = BuildStatement(
                StringExtensions::sprintf(
                    "INSERT INTO users (%s) VALUES (%s)",
                    userColumnNames.str().c_str(),
                    userColumnValuePlaceholders.str().c_str()
                )
            );
            if (createUser == nullptr) {
                CloseDatabase();
                return false;
            }
            addChat = BuildStatement(
                "INSERT INTO chat (userid, message) VALUES (?, ?)"
            );
            if (addChat == nullptr) {
                CloseDatabase();
                return false;
            }
            dropOldChat = BuildStatement(
                "DELETE"
                " FROM chat"
                " WHERE userid = ?1"
                " AND seq NOT IN ("
                    "SELECT seq"
                    " FROM chat"
                    " WHERE userid = ?1"
                    " ORDER BY seq DESC LIMIT ?2"
                " )"
            );
            if (dropOldChat == nullptr) {
                CloseDatabase();
                return false;
            }
            return true;
        }

        void ResetStatement(const PreparedStatement& stmt) {
            (void)sqlite3_reset(stmt.get());
        }

        bool StepStatement(const PreparedStatement& stmt) {
            bool result = false;
            const auto stepResult = sqlite3_step(stmt.get());
            switch (stepResult) {
                case SQLITE_DONE: {
                } break;

                case SQLITE_ROW: {
                    result = true;
                } break;

                default: {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                        "Error stepping SQL statement (%d): %s",
                        stepResult,
                        sqlite3_errmsg(db.get())
                    );
                } break;
            }
            return result;
        }
    };

    UsersStore::~UsersStore() noexcept = default;
    UsersStore::UsersStore(UsersStore&&) noexcept = default;
    UsersStore& UsersStore::operator=(UsersStore&&) noexcept = default;

    UsersStore::UsersStore()
        : impl_(new Impl())
    {
    }

    void UsersStore::CreateUser(
        User::Bot bot,
        double createdAt,
        double firstMessageTime,
        double firstSeenTime,
        intmax_t id,
        bool isBanned,
        bool isWhitelisted,
        double lastMessageTime,
        const std::string& login,
        const std::string& name,
        const std::string& note,
        intmax_t numMessages,
        User::Role role,
        double timeout,
        double totalViewTime,
        bool watching
    ) {
        impl_->ResetStatement(impl_->createUser);
        impl_->BindStatementParameter(impl_->createUser, 1, bot);
        impl_->BindStatementParameter(impl_->createUser, 2, createdAt);
        impl_->BindStatementParameter(impl_->createUser, 3, firstMessageTime);
        impl_->BindStatementParameter(impl_->createUser, 4, firstSeenTime);
        impl_->BindStatementParameter(impl_->createUser, 5, id);
        impl_->BindStatementParameter(impl_->createUser, 6, isBanned);
        impl_->BindStatementParameter(impl_->createUser, 7, isWhitelisted);
        impl_->BindStatementParameter(impl_->createUser, 8, lastMessageTime);
        impl_->BindStatementParameter(impl_->createUser, 9, login);
        impl_->BindStatementParameter(impl_->createUser, 10, name);
        impl_->BindStatementParameter(impl_->createUser, 11, note);
        impl_->BindStatementParameter(impl_->createUser, 12, numMessages);
        impl_->BindStatementParameter(impl_->createUser, 13, role);
        impl_->BindStatementParameter(impl_->createUser, 14, timeout);
        impl_->BindStatementParameter(impl_->createUser, 15, totalViewTime);
        impl_->BindStatementParameter(impl_->createUser, 16, watching);
        (void)impl_->StepStatement(impl_->createUser);
    }

    void UsersStore::UpdateUserBot(
        intmax_t id,
        User::Bot bot
    ) {
        const auto& statement = impl_->updateUser["bot"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, bot);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserCreatedAt(
        intmax_t id,
        double createdAt
    ) {
        const auto& statement = impl_->updateUser["createdAt"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, createdAt);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserFirstMessageTime(
        intmax_t id,
        double firstMessageTime
    ) {
        const auto& statement = impl_->updateUser["firstMessageTime"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, firstMessageTime);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserFirstSeenTime(
        intmax_t id,
        double firstSeenTime
    ) {
        const auto& statement = impl_->updateUser["firstSeenTime"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, firstSeenTime);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserIsBanned(
        intmax_t id,
        bool isBanned
    ) {
        const auto& statement = impl_->updateUser["isBanned"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, isBanned);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserIsWhitelisted(
        intmax_t id,
        bool isWhitelisted
    ) {
        const auto& statement = impl_->updateUser["isWhitelisted"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, isWhitelisted);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserLastMessageTime(
        intmax_t id,
        double lastMessageTime
    ) {
        const auto& statement = impl_->updateUser["lastMessageTime"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, lastMessageTime);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserLogin(
        intmax_t id,
        const std::string& login
    ) {
        const auto& statement = impl_->updateUser["login"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, login);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserName(
        intmax_t id,
        const std::string& name
    ) {
        const auto& statement = impl_->updateUser["name"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, name);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserNote(
        intmax_t id,
        const std::string& note
    ) {
        const auto& statement = impl_->updateUser["note"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, note);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserNumMessages(
        intmax_t id,
        intmax_t numMessages
    ) {
        const auto& statement = impl_->updateUser["numMessages"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, numMessages);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserRole(
        intmax_t id,
        User::Role role
    ) {
        const auto& statement = impl_->updateUser["role"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, role);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserTimeout(
        intmax_t id,
        double timeout
    ) {
        const auto& statement = impl_->updateUser["timeout"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, timeout);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserTotalViewTime(
        intmax_t id,
        double totalViewTime
    ) {
        const auto& statement = impl_->updateUser["totalViewTime"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, totalViewTime);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::UpdateUserWatching(
        intmax_t id,
        bool watching
    ) {
        const auto& statement = impl_->updateUser["watching"];
        impl_->ResetStatement(statement);
        impl_->BindStatementParameter(statement, 2, id);
        impl_->BindStatementParameter(statement, 1, watching);
        (void)impl_->StepStatement(statement);
    }

    void UsersStore::AddChat(
        intmax_t userId,
        const std::string& message,
        size_t maxUserChatLines
    ) {
        impl_->ResetStatement(impl_->addChat);
        impl_->BindStatementParameter(impl_->addChat, 1, userId);
        impl_->BindStatementParameter(impl_->addChat, 2, message);
        (void)impl_->StepStatement(impl_->addChat);
        impl_->ResetStatement(impl_->dropOldChat);
        impl_->BindStatementParameter(impl_->dropOldChat, 1, userId);
        impl_->BindStatementParameter(impl_->dropOldChat, 2, (intmax_t)maxUserChatLines);
        (void)impl_->StepStatement(impl_->dropOldChat);
    }

    void UsersStore::Add(const User& user) {
        impl_->userIdsByLogin[user.login] = user.id;
        auto userStore = std::make_shared< UserStore >(user, shared_from_this());
        userStore->Create();
        impl_->usersById[user.id] = std::move(userStore);
    }

    std::shared_ptr< UserStore > UsersStore::FindById(intmax_t id) {
        auto usersByIdEntry = impl_->usersById.find(id);
        if (usersByIdEntry == impl_->usersById.end()) {
            return nullptr;
        } else {
            return usersByIdEntry->second;
        }
    }

    std::shared_ptr< UserStore > UsersStore::FindByLogin(const std::string& login) {
        const auto userIdsByLoginEntry = impl_->userIdsByLogin.find(login);
        if (userIdsByLoginEntry == impl_->userIdsByLogin.end()) {
            return nullptr;
        } else {
            const auto userid = userIdsByLoginEntry->second;
            auto usersByIdEntry = impl_->usersById.find(userid);
            if (usersByIdEntry == impl_->usersById.end()) {
                return nullptr;
            } else {
                return usersByIdEntry->second;
            }
        }
    }

    void UsersStore::Migrate(const Json::Value& jsonUsers) {
        const auto numUsers = jsonUsers.GetSize();
        for (size_t i = 0; i < numUsers; ++i) {
            const auto& userEncoded = jsonUsers[i];
            const auto userid = (intmax_t)(size_t)userEncoded["id"];
            User user;
            user.id = userid;
            user.login = (std::string)userEncoded["login"];
            user.name = (std::string)userEncoded["name"];
            user.createdAt = (double)userEncoded["createdAt"];
            user.totalViewTime = (double)userEncoded["totalViewTime"];
            user.firstSeenTime = (double)userEncoded["firstSeenTime"];
            user.firstMessageTime = (double)userEncoded["firstMessageTime"];
            user.lastMessageTime = (double)userEncoded["lastMessageTime"];
            user.numMessages = (size_t)userEncoded["numMessages"];
            user.timeout = (double)userEncoded["timeout"];
            user.isBanned = (bool)userEncoded["isBanned"];
            user.isWhitelisted = (bool)userEncoded["isWhitelisted"];
            user.watching = (bool)userEncoded["watching"];
            user.note = (std::string)userEncoded["note"];
            if (userEncoded.Has("bot")) {
                const auto bot = (std::string)userEncoded["bot"];
                if (bot == "yes") {
                    user.bot = User::Bot::Yes;
                } else if (bot == "no") {
                    user.bot = User::Bot::No;
                }
            }
            if (userEncoded.Has("role")) {
                const auto role = (std::string)userEncoded["role"];
                if (role == "staff") {
                    user.role = User::Role::Staff;
                    user.isWhitelisted = true;
                } else if (role == "admin") {
                    user.role = User::Role::Admin;
                    user.isWhitelisted = true;
                } else if (role == "broadcaster") {
                    user.role = User::Role::Broadcaster;
                    user.isWhitelisted = true;
                } else if (role == "moderator") {
                    user.role = User::Role::Moderator;
                    user.isWhitelisted = true;
                } else if (role == "vip") {
                    user.role = User::Role::VIP;
                    user.isWhitelisted = true;
                } else if (role == "pleb") {
                    user.role = User::Role::Pleb;
                }
            }
            const auto& lastChat = userEncoded["lastChat"];
            const auto numLastChatLines = lastChat.GetSize();
            for (size_t j = 0; j < numLastChatLines; ++j) {
                user.lastChat.push_back((std::string)lastChat[j]);
            }
            Add(user);
        }
    }

    bool UsersStore::Mobilize(const std::string& dbFilePath) {
        impl_->dbFilePath = dbFilePath;
        if (!impl_->OpenDatabase()) {
            return false;
        }
        impl_->GetUsers(shared_from_this());
        return true;
    }

    void UsersStore::SetUserId(const std::string& login, intmax_t id) {
        auto usersByIdEntry = impl_->usersById.find(id);
        if (usersByIdEntry == impl_->usersById.end()) {
            User user;
            user.id = id;
            user.login = login;
            Add(user);
        } else {
            impl_->userIdsByLogin[login] = id;
            auto& user = usersByIdEntry->second;
            if (user->GetLogin() != login) {
                impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
                    3,
                    "Twitch user %" PRIdMAX " login changed from %s to %s",
                    id,
                    user->GetLogin().c_str(),
                    login.c_str()
                );
                (void)impl_->userIdsByLogin.erase(user->GetLogin());
                user->SetLogin(login);
            }
        }
    }

    SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate UsersStore::SubscribeToDiagnostics(
        SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
        size_t minLevel
    ) {
        return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    void UsersStore::WithAll(
        std::function< void(const std::shared_ptr< UserStore >&) > visitor
    ) {
        for (const auto usersByIdEntry: impl_->usersById) {
            visitor(usersByIdEntry.second);
        }
    }

}
