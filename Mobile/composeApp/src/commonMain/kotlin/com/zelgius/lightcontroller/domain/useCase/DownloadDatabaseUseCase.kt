package com.zelgius.lightcontroller.domain.useCase

import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import org.koin.core.annotation.Factory
import org.koin.core.annotation.Provided

@Factory
class DownloadDatabaseUseCase(
    @Provided private val serverRepository: ServerRepository,
    @Provided private val factory: com.zelgius.lightcontroller.Factory
) {

    suspend operator fun invoke(): Boolean {
        val bytes = serverRepository.exportDatabase()

        return  factory.download(bytes, "lights.db", "application/vnd.sqlite3" )
    }
}