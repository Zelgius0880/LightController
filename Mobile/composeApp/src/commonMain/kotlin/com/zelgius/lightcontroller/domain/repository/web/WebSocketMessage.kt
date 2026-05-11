package com.zelgius.lightcontroller.domain.repository.web

import io.ktor.serialization.JsonConvertException
import kotlinx.datetime.LocalDate
import kotlinx.datetime.LocalDateTime
import kotlinx.datetime.LocalTime
import kotlinx.datetime.format.char
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json

sealed interface WebSocketMessage {

    companion object {

        val dateFormat = LocalDateTime.Format {
            date(LocalDate.Formats.ISO) // Handles YYYY-MM-DD
            char(' ')                           // Handles the space
            time(LocalTime.Formats.ISO) // Handles HH:MM:SS
        }

        fun fromString(s: String): WebSocketMessage {
            return try {
                Json.decodeFromString<Status>(s)
            } catch (e: Exception) {
                e.printStackTrace()

                val regex = Regex("""^(.{19}):\s(.*)$""")
                val matchResult = regex.find(s)

                if (matchResult != null) {
                    val (timeString, logMessage) = matchResult.destructured
                    Log(time = dateFormat.parseOrNull(timeString), message = logMessage.trimEnd())

                } else
                    Log(time = null, message = s.trimEnd())

            }
        }
    }

    @Serializable
    data class Log(
        val time: LocalDateTime?,
        val message: String
    ) : WebSocketMessage

    @Serializable
    data class Status(
        val authenticated: Boolean = false,
        val username: String = "",
        val totalBytes: Int = 0,
        val usedBytes: Int = 0,
        val netatmo: Netatmo = Netatmo(),
        val fsTotal: Int = 0,
        val fsUsed: Int = 0,
        val firmware: String = "",
        val heapTotal: Int = 0,
        val heapFree: Int = 0
    ) : WebSocketMessage {
        @Serializable
        data class Netatmo(
            val authenticated: Boolean = false,
            @SerialName("expires_in")
            val expireIn: Long = 0,

            @SerialName("creation_timestamp")
            val creationTimestamp: Long = 0,
            val valid: Boolean = false,
        )
    }
}