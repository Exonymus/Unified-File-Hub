package com.relsa.desurep.repositories;

import com.relsa.desurep.models.File;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface FileRepository extends JpaRepository<File, Long> {
    File findFileByFilename(String filename);
    File findFileByDownloadId(String downloadId);
    List<File> findFilesByUploaderName(String uploaderName);
    List<File> findFilesByIsPublic(Boolean isPublic);
}
