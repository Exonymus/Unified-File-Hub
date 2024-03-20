package com.relsa.desurep;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.domain.EntityScan;
import org.springframework.data.jpa.repository.config.EnableJpaRepositories;

@SpringBootApplication
@EnableJpaRepositories
@EntityScan("com.relsa.desurep.models")
public class FileRepositoryApplication {

	public static void main(String[] args) {

		SpringApplication.run(FileRepositoryApplication.class, args);
	}

}
