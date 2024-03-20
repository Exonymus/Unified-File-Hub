package com.relsa.desurep.services;

import com.relsa.desurep.models.File;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;

@Service
public class FileSerchService {
    public List<File> initSerch(List<File> startSample, File exFile) {
        List<File> resultSample = new ArrayList<>();
        for(File file: startSample)
            if(exFile.equals(file))
                resultSample.add(file);

        return resultSample;
    }

    public File createExemplaryFile(String publicationName, String authorName,
                                     String theme, String publicationDate,
                                     String uploadDate) {
        String exPublicationName, exAuthorName, exTheme, exPublicationDate,
                exUploadDate;
        if (publicationName == null || publicationName.isEmpty()) exPublicationName = "Any_value";
        else exPublicationName = publicationName;

        if (authorName == null || authorName.isEmpty()) exAuthorName = "Any_value";
        else exAuthorName = authorName;

        if (theme == null || theme.isEmpty()) exTheme = "Any_value";
        else exTheme = theme;

        if (publicationDate == null || publicationDate.isEmpty()) exPublicationDate = "Any_value";
        else exPublicationDate = publicationDate;

        if (uploadDate == null || uploadDate.isEmpty()) exUploadDate = "Any_value";
        else exUploadDate = uploadDate;

        return new File(exPublicationName, exAuthorName, exTheme, exPublicationDate,
                exUploadDate);
    }
}
