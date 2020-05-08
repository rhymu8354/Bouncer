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
        {"createdAt", "REAL"},
        {"firstMessageTime", "REAL"},
        {"firstSeenTime", "REAL"},
        {"id", "INTEGER"},
        {"isBanned", "BOOLEAN"},
        {"isWhitelisted", "BOOLEAN"},
        {"lastMessageTime", "REAL"},
        {"login", "TEXT"},
        {"name", "TEXT"},
        {"note", "TEXT"},
        {"numMessages", "INTEGER"},
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
        // Properties

        PreparedStatement createUser;
        DatabaseConnection db;
        std::string dbFilePath;

        /**
         * This is a helper object used to generate and publish
         * diagnostic messages.
         */
        SystemAbstractions::DiagnosticsSender diagnosticsSender;

        PreparedStatement updateUser;
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

        void CloseDatabase() {
            createUser = nullptr;
            updateUser = nullptr;
            db = nullptr;
        }

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
            }
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
            updateUser = BuildStatement(
                "UPDATE users SET ? = ? WHERE id = ?"
            );
            if (updateUser == nullptr) {
                CloseDatabase();
                return false;
            }
            return true;
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

    };

    UsersStore::~UsersStore() noexcept = default;
    UsersStore::UsersStore(UsersStore&&) noexcept = default;
    UsersStore& UsersStore::operator=(UsersStore&&) noexcept = default;

    UsersStore::UsersStore()
        : impl_(new Impl())
    {
    }

    void UsersStore::Add(const User& user) {
        impl_->userIdsByLogin[user.login] = user.id;
        auto userStore = std::make_shared< UserStore >();
        userStore->Connect(shared_from_this());
        userStore->Create(user);
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
        impl_->CloseDatabase();
        SystemAbstractions::File dbFile(impl_->dbFilePath);
        dbFile.Destroy();
        if (!impl_->OpenDatabase()) {
            return;
        }
        impl_->usersById.clear();
        impl_->userIdsByLogin.clear();
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
        return impl_->OpenDatabase();
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

}
